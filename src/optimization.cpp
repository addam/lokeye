#include "main.h"
#include "bitmap.h"
#include "optimization.h"
#include "homography.h"
#include <iostream>
#include <random>
#include <mutex>
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

Gaze::Gaze(const Matrix35 &fn) : fn(fn)
{
}

Gaze Gaze::ransac(const vector<Measurement> &pairs, int &out_support, float precision)
{
    const int necessary_support = out_support;
    out_support = 0;
    const int min_sample = 7;
    float iterations = 1 + combinations_ratio<min_sample>(pairs.size(), necessary_support);
    Matrix35 result;
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
            result = h;
            if (sample.size() >= necessary_support) {
                iterations = combinations_ratio<min_sample>(pairs.size(), sample.size());
            }
        }
    }
    return Gaze(result);
}

Gaze Gaze::guess(Safe<vector<Measurement>> &pairs, int &out_support, float precision)
{
    using Lock = std::lock_guard<Safe<vector<Measurement>>>;    
    const int necessary_support = out_support;
    out_support = 0;
    vector<Measurement> sample;
    {
        Lock lk(pairs);
        sample = random_sample<7>(pairs);
    }
    Matrix35 h = homography<3, 5>(sample);
    do {
        out_support = sample.size();
        h = homography<3, 5>(sample);
        {
            Lock lk(pairs);
            sample = support(h, pairs, precision);
        }
    } while (sample.size() > out_support);
    return Gaze(h);
}

Vector2 Gaze::operator () (Vector4 v) const
{
    return project(v, fn);
}

Face::Face(const Bitmap3 &ref, Region region, Circle left_eye, Circle right_eye):
    ref{ref.clone()},
    main_tsf{region},
    children{this->ref, region},
    eyes{left_eye, right_eye}
{
}

Transformation::Params update_step(const Transformation &tsf, const Bitmap3 &img, const Bitmap3 &grad, const Bitmap3 &ref, int direction)
{
    Transformation::Params result;
    // formula : delta_tsf = -sum_pixel (img o tsf - ref)^t * gradient(img o tsf) * gradient(tsf)
    Transformation tsf_inv = tsf.inverse();
    for (Pixel p : sampling(grad, tsf.region)) {
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
    for (Pixel p : sampling(reference, tsf.region)) {
        Vector2 v = tsf(reference.to_world(p));
        Vector3 diff = img(v) - reference(p);
        result += diff.dot(diff);
        if (not (std::isfinite(result) and result >= 0)) {
            std::cout << "img@" << v << " = " << img(v) << std::endl;
            std::cout << "ref@" << p << " = " << reference(p) << std::endl;
            std::cout << diff << "**2 = " << diff.dot(diff) << std::endl;
            assert(false);
        }
        assert(std::isfinite(result) and result >= 0);
    }
    return 0.5 * result;
}

/** Maximum difference caused by adding 'step' to 'tsf', in pixels
 * @param step Differential update of the transformation
 */
float step_length(Transformation::Params step, const Transformation &tsf)
{
    float result = 0;
    for (Vector2 v : tsf.vertices()) {
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

float line_search(Transformation::Params delta_tsf, float &prev_energy, float length, const Transformation &tsf, const Bitmap3 &img, const Bitmap3 &ref)
{
    const int iteration_count = 2;
    const float epsilon = 1e-5;
    float current_energy = evaluate(tsf + length * delta_tsf, img, ref);
    for (int i=0; i<iteration_count; ++i) {
        float slope = -delta_tsf.dot(delta_tsf) * length;
        if (slope > 0) {
			printf("prev: %g, current: %g, slope: %g, fail.\n", prev_energy, current_energy, slope);
			return 0;
		}
        float coef = quadratic_minimum(prev_energy, current_energy, slope);
        float next_energy = evaluate(tsf + (coef * length) * delta_tsf, img, ref);
        if (next_energy / current_energy > 1 - epsilon or coef == 1) {
            prev_energy = current_energy;
            return length;
        } else {
            length *= coef;
            current_energy = next_energy;
        }
    }
    return length;
}

void refit_transformation(Transformation &tsf, const Bitmap3 &img, const Bitmap3 &ref, int min_size)
{
    const int iteration_count = 2;
    vector<std::pair<Bitmap3, Bitmap3>> pyramid = {{img, ref}};
    for (float size=radius(tsf.region); size > min_size; size /= 2) {
        auto &pair = pyramid.back();
        pyramid.emplace_back(pair.first.downscale(), pair.second.downscale());
    }
    std::reverse(pyramid.begin(), pyramid.end());
    for (const auto &pair : pyramid) {
        Bitmap3 dx = pair.first.d(0), dy = pair.first.d(1);
        float prev_energy = evaluate(tsf, pair.first, pair.second);
        for (int iteration=0; iteration < iteration_count; ++iteration) {
            Transformation::Params delta_tsf = update_step(tsf, pair.first, dx, pair.second, 0) + update_step(tsf, pair.first, dy, pair.second, 1);
            float step_mag = 2 * step_length(delta_tsf, tsf);
            if (step_mag < 1e-10) {
                break;
            }
            float length = line_search(delta_tsf, prev_energy, pair.first.scale / step_mag, tsf, pair.first, pair.second);
            if (length > 0) {
                tsf += length * delta_tsf;
            } else {
                break;
            }
        }
    }
}

void Face::refit(const Bitmap3 &img, bool only_eyes)
{
    if (not only_eyes) {
        refit_transformation(main_tsf, img, ref, 5);
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
    if (face_cl.empty() or eye_cl.empty()) {
		throw std::runtime_error("Face classification parameters could not be loaded. Check that the paths in optimization.h are correct.");
	}
    
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
        eyes[i].center = image.to_world(parent.tl() + center(r));
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
		Gaze result = Gaze::ransac(measurements, support, precision);
		for (Measurement pair : measurements) {
			std::cout << pair.first << " -> " << result(pair.first) << " vs. " << pair.second << ((cv::norm(result(pair.first) - pair.second) < precision) ? " INLIER" : " out") << std::endl;
		}
		if (support >= necessary_support) {
			return result;
		}
	}	
}
