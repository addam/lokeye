#include "main.h"
#include "bitmap.h"
#include "optimization.h"
#include "homography.h"
#include <iostream>
#include <random>
#include <opencv2/objdetect.hpp>

template<int size>
vector<Measurement> random_sample(const vector<Measurement> &pairs)
{
    if (pairs.size() <= size) {
        return vector<Measurement>(pairs.begin(), pairs.end());
    }
    std::array<int, size> indices;
    std::random_device rd;
    std::minstd_rand generator(rd());
    for (int i=0; i<size; i++) {
        int high = pairs.size() - size + i;
        auto it = indices.begin() + i;
        int index = std::uniform_int_distribution<>(0, high - 1)(generator);
        *it = (std::find(indices.begin(), it, index) == it) ? index : high;
    }
    vector<Measurement> result;
    result.reserve(size);
    std::transform(indices.begin(), indices.end(), std::back_inserter(result), [pairs](int index) { return pairs[index]; });
    return result;
}

vector<Measurement> support(const Matrix35 h, const vector<Measurement> &pairs, const float precision)
{
    vector<Measurement> result;
    std::copy_if(pairs.begin(), pairs.end(), std::back_inserter(result), [h, precision](Measurement p) { return cv::norm(project(p.first, h) - p.second) < precision; });
    return result;
}

template<int count>
float combinations_ratio(int count_total, int count_good)
{
    const float logp = -1;
    float result = logp / std::log(1 - pow(count_good / float(count_total), count));
    printf(" need %g samples (%i over %i)\n", result, count_good, count_total);
    return result;
}

Gaze::Gaze(const vector<Measurement> &pairs, int &out_support, float precision)
{
    if (false) {
        vector<Measurement> subpairs = random_sample<8>(pairs);//(pairs.begin(), pairs.begin() + 7);
        fn = homography<3, 5>(subpairs);
        out_support = support(fn, pairs, precision).size();
        printf("support = %i / %lu, %f %f %f %f %f; %f %f %f %f %f; %f %f %f %f %f\n", out_support, pairs.size(), fn(0,0), fn(0,1), fn(0,2), fn(0,3), fn(0,4), fn(1,0), fn(1,1), fn(1,2), fn(0,3), fn(0,4), fn(2,0), fn(2,1), fn(2,2), fn(0,3), fn(0,4));
        return;
    }
    const int necessary_support = std::min(out_support, int(pairs.size()));
    out_support = 0;
    const int min_sample = 7;
    float iterations = 1 + combinations_ratio<min_sample>(pairs.size(), necessary_support);
    for (int i=0; i < iterations; i++) {
        vector<Measurement> sample = random_sample<min_sample>(pairs);
        size_t prev_sample_size;
        Matrix35 h = homography<3, 5>(sample);
        do {
            prev_sample_size = sample.size();
            h = homography<3, 5>(sample);
            sample = support(h, pairs, precision);
        } while (sample.size() > prev_sample_size);
        if (sample.size() > out_support) {
            out_support = sample.size();
            fn = h;
            if (sample.size() >= necessary_support) {
                iterations = combinations_ratio<min_sample>(pairs.size(), sample.size());
            }
        }
    }
    printf("support = %i / %lu, %f %f %f %f %f; %f %f %f %f %f; %f %f %f %f %f\n", out_support, pairs.size(), fn(0,0), fn(0,1), fn(0,2), fn(0,3), fn(0,4), fn(1,0), fn(1,1), fn(1,2), fn(0,3), fn(0,4), fn(2,0), fn(2,1), fn(2,2), fn(0,3), fn(0,4));
}

Vector2 Gaze::operator () (Vector4 v) const
{
    return project(v, fn);
}

Face::Face(const Bitmap3 &ref, Region main_region, Circle left_eye, Circle right_eye):
    ref{ref.clone()},
    main_tsf{main_region},
    main_region{main_region},
    children{ref, main_region},
    eyes{left_eye, right_eye}
{
}

Transformation::Params update_step(const Transformation &tsf, const Bitmap3 &img, const Bitmap3 &grad, const Bitmap3 &ref, int direction)
{
    Transformation::Params result;
    // formula : delta_tsf = -sum_pixel (img o tsf - ref)^t * gradient(img o tsf) * gradient(tsf)
    Transformation tsf_inv = tsf.inverse();
    for (Pixel p : grad) {
        Vector2 v = grad.to_world(p), refv = tsf_inv(v);
        if (ref.contains(refv)) {
            Vector3 diff = img(v) - ref(refv);
            result -= diff.dot(grad(p)) * tsf.d(refv, direction);
        }
    }
    return result;
}

float evaluate(const Transformation &tsf, const Bitmap3 &img, const Bitmap3 &reference)
{
    float result = 0;
    // formula : energy = 1/2 * sum_pixel (img o tsf - ref)^2
    for (Pixel p : reference) {
        Vector2 v = tsf(reference.to_world(p));
        Vector3 diff = img(v) - reference(p);
        result += diff.dot(diff);
    }
    return 0.5 * result;
}

/** Maximum difference caused by adding 'step' to 'tsf', in pixels
 * @param step Differential update of the transformation
 * @param r Region of interest (search domain for the maximum)
 */
float step_length(Transformation::Params step, Region r, const Transformation &tsf)
{
    Vector2 points[] = {to_vector(r.tl()), Vector2(r.tl().x, r.br().y), to_vector(r.br()), Vector2(r.br().x, r.tl().y)};
    float result = 1e-5;
    for (Vector2 v : points) {
        for (int i=0; i<2; i++) {
            result = std::max(result, std::abs(tsf.d(v, i).dot(step)));
        }
    }
    return result;
}

/** Minimum argument of a quadratic function
 * function F satisfies: F(0) = f0, F(1) = f1, F'(0) = df0
 * @returns x in range (0...1) such that F(x) is minimal
 */
float quadratic_minimum(float f0, float f1, float df0)
{
	if (df0 >= 0) {
		//std::clog << "quadratic_minimum panic!" << std::endl;
		return 0;
	}
    float second_order = f1 - f0 - df0;
    float minx = -0.5 * df0 / second_order;
    if (second_order < 0) {
        assert (minx < 0);
        assert (f1 < f0);
    }
    if (minx > 1) {
        assert(f1 < f0);
    }
    if (second_order < 0 or minx > 1) {
        return 1;
    } else {
        return minx;
    }
}

float line_search(Transformation::Params delta_tsf, float &max_length, float &prev_energy, const Transformation &tsf, const Bitmap3 &img, const Bitmap3 &ref)
{
    const float epsilon = 1e-5;
    float length = max_length;
    float current_energy = evaluate(tsf + length * delta_tsf, img, ref);
    while (1) {
        float slope = -delta_tsf.dot(delta_tsf) * length;
        if (slope > 0) {
			printf("prev: %g, current: %g, slope: %g, fail.\n", prev_energy, current_energy, slope);
			return 0;
		}
        float coef = quadratic_minimum(prev_energy, current_energy, slope);
        float next_energy = evaluate(tsf + coef * length * delta_tsf, img, ref);
        if (next_energy >= current_energy or coef == 1) {
            max_length = 2 * length;
            prev_energy = current_energy;
            return length;
        } else if (next_energy / current_energy > 1 - epsilon) {
            return 0;
        } else {
            length *= coef;
            current_energy = next_energy;
        }
    }
}

void refit_transformation(Transformation &tsf, Region region, const Bitmap3 &img, const Bitmap3 &ref, int min_size)
{
    const int iteration_count = 2;
    Region rotregion = tsf(region);
    vector<std::pair<Bitmap3, Bitmap3>> pyramid{std::make_pair(img.crop(rotregion), ref.crop(region))};
    if (std::min({pyramid.back().first.rows, pyramid.back().first.cols, pyramid.back().second.rows, pyramid.back().second.cols}) == 0) {
        return;
    }
    while (std::min({pyramid.back().first.rows, pyramid.back().first.cols, pyramid.back().second.rows, pyramid.back().second.cols}) >= 2 * min_size) {
        auto &pair = pyramid.back();
        pyramid.emplace_back(pair.first.downscale(), pair.second.downscale());
    }
    while (not pyramid.empty()) {
        auto &pair = pyramid.back();
        Bitmap3 dx = pair.first.d(0), dy = pair.first.d(1);
        float prev_energy = evaluate(tsf, pair.first, pair.second);
        //printf("evaluate (%ix%i) vs. (%ix%i) yields %g\n", pair.first.cols, pair.first.rows, pair.second.cols, pair.second.rows, prev_energy);
        if (prev_energy > 0) {
	        float max_length;
	        for (int iteration=0; iteration < iteration_count; ++iteration) {
	            Transformation::Params delta_tsf = update_step(tsf, pair.first, dx, pair.second, 0) + update_step(tsf, pair.first, dy, pair.second, 1);
	            if (iteration == 0) {
	                max_length = (1 << pyramid.size()) / step_length(delta_tsf, rotregion, tsf);
	            }
	            float length = line_search(delta_tsf, max_length, prev_energy, tsf, pair.first, pair.second);
	            if (length > 0) {
	                tsf += delta_tsf * length;
	            } else {
	                break;
	            }
	        }
		}
        pyramid.pop_back();
    }
}

void Face::refit(const Bitmap3 &img, bool only_eyes)
{
    if (not only_eyes) {
        refit_transformation(main_tsf, main_region, img, ref, 5);
        children.refit(img, main_tsf);
    }
    for (int i=0; i<2; ++i) {
        if (not eye_locator) {
            static bool has_notified = false;
            if (not has_notified) {
                has_notified = true;
                fprintf(stderr, "Eye tracking has not been set up.\n");
            }
        } else {
            ///@todo Implement Transformation::operator() (Circle)
            Circle view_eye{main_tsf(eyes[i].center), eyes[i].radius * main_tsf.scale(eyes[i].center)};
            eye_locator->refit(view_eye, img);
            fitted_eyes[i] = {main_tsf.inverse(view_eye.center), eyes[i].radius};
        }
    }
}

Vector4 Face::operator () () const
{
    const Vector2 e = fitted_eyes[0].center + fitted_eyes[1].center;
    /// @note here, we are assuming that Transformation is linear
    Vector2 difference = children(main_tsf);
    return Vector4(e[0], e[1], difference[0], difference[1]);
}

Face init_static(const Bitmap3 &image, const string &face_xml, const string &eye_xml)
{
    using CharMat = cv::Mat_<unsigned char>;
    cv::CascadeClassifier face_cl(face_xml);
    cv::CascadeClassifier eye_cl(eye_xml);
    
    CharMat gray;
    cv::Mat tmp;
    cv::cvtColor(image, tmp, cv::COLOR_BGR2GRAY);
    tmp.convertTo(gray, CV_8U, 255);
    equalizeHist(gray, gray);

    std::vector<Rect> faces;
    face_cl.detectMultiScale(gray, faces, 1.1, 2, cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));
    if (faces.empty()) {
        throw NoFaceException();
    }
    Rect parent = faces.front();
    float scale = parent.height;
    std::vector<Rect> eye_rects;
    CharMat crop = gray(parent);
    eye_cl.detectMultiScale(crop, eye_rects, 1.1, 2, cv::CASCADE_SCALE_IMAGE, cv::Size(scale/6, scale/6), cv::Size(scale/4, scale/4));
    if (eye_rects.size() < 2) {
        throw NoFaceException();
    }
    std::array<Circle, 2> eyes;
    for (int i=0; i<2; ++i) {
        Rect r = eye_rects[i];
        eyes[i].center = image.to_world(parent.tl() + (r.tl() + r.br()) / 2);
        eyes[i].radius = 0.04 * scale;
    }
    ///@todo fixme init grid children if asked for it
    return Face{image, to_region(parent), eyes[0], eyes[1]};
}

Gaze calibrate_static(Face &state, VideoCapture &cap, TrackingData::const_iterator &it)
{
    const int necessary_support = 80;
    vector<Measurement> measurements;
    while (1) {
		for (int i = 0; i < necessary_support; ++i) {
			Bitmap3 image;
			image.read(cap);
			state.refit(image);
	        std::cout << " refitted " << state() << std::endl;
			measurements.emplace_back(state(), *(it++));
		}
		int support = necessary_support;
		const float precision = 150;
		std::cout << "starting to solve..." << std::endl;
		Gaze result(measurements, support, precision);
		for (Measurement pair : measurements) {
			std::cout << pair.first << " -> " << result(pair.first) << " vs. " << pair.second << ((cv::norm(result(pair.first) - pair.second) < precision) ? " INLIER" : " out") << std::endl;
		}
		if (support >= necessary_support) {
			return result;
		}
	}	
}
