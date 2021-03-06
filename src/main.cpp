// ♡2015-2017 Adam Dominec. Please share and reuse
#define WITH_OPENMP
#include <iostream>
#include <fstream>
#include "main.h"
#include "bitmap.h"
#include "optimization.h"

bool inside(Vector2 pos, Pixel size)
{
    return pos(0) >= 0 and pos(1) >= 0 and pos(0) < size.x and pos(1) < size.y;
}

string replace_extension(const string &filename, const string &extension)
{
	return filename.substr(0, filename.rfind('.')) + extension;
}

bool is_numeric(const string str)
{
	return std::all_of(str.begin(), str.end(), [](char c) { return std::isdigit(c); });
}

void track_interactive(Face &state, VideoCapture &cam, Gaze &fit, Pixel size)
{
    const Vector3 bg_color(0.4, 0.3, 0.3);
    Bitmap3 record(size.y, size.x);
    record = bg_color;
    Bitmap3 image;
    Vector2 prev_pos(-1, -1);
    for (int i=0; char(cv::waitKey(5)) != 27 and image.read(cam); i++) {
        state.refit(image);
        Vector2 pos = fit(state());
        //std::cout << "position: " << pos(0) << ", " << pos(1) << std::endl;
        state.render(image, "tracking");
        if (inside(pos, size) and inside(prev_pos, size)) {
            cv::line(record, to_pixel(prev_pos), to_pixel(pos), cv::Scalar(0.5, 0.7, 1));
        }
        if (i % 2 == 0) {
            cv::imshow("record", record);
            const float decay = 0.1;
            record = decay * bg_color + (1 - decay) * record;
        }
        prev_pos = pos;
    }
}

TrackingData track_static(Face &state, VideoCapture &cam, Gaze &fit, TrackingData::const_iterator &it)
{
	TrackingData result;
	Bitmap3 image;
	while (image.read(cam)) {
        state.refit(image);
        Vector2 pos = fit(state());
		std::clog << "difference " << cv::norm(*it - pos) <<  ", estimated " << pos << ", truth " << *it << std::endl;
		++it;
        result.push_back(pos);
	}
	return result;
}

TrackingData read_csv(const string &filename)
{
	TrackingData result;
	std::ifstream is(filename);
	while (1) {
		string line;
		std::getline(is, line);
		std::stringstream line_s(line);
		Vector2 value;
		for (int i=0; i<2; ++i) {
			string cell;
			std::getline(line_s, cell, ',');
			if (cell.empty()) {
				return result;
			}
			value[i] = std::stof(cell);
		}
		result.push_back(value);
	}
}

float average_difference(const TrackingData &left, const TrackingData &right)
{
	assert (left.size() <= right.size());
	float sum;
	for (int i=0; i<left.size(); ++i) {
		sum += cv::norm(left[i] - right[i]);
	}
	return sum / left.size();
}

void set_eye_finder(Face& face)
{
    auto serial = new SerialEye;
    FindEyePtr hough(new HoughEye);
    FindEyePtr limbus(new LimbusEye);
    serial->add(std::move(hough));
    serial->add(std::move(limbus));
    face.eye_locator.reset(serial);
}

void display_help()
{
	printf("Usage: fit_eyes [-i] [-v] [index of webcam] [video.avi [ground_truth.csv]]\n");
	printf("\t-i:\tinteractive (mark the face by hand)\n");
	printf("\t-v:\tverbose\n");
}

int main(int argc, char** argv)
{
	string video_filename, csv_filename;
	int camera_index = 0;
	int frame_begin = 0, frame_step = 1;
	std::vector<int> numeric_args;
	bool is_interactive = false, is_verbose = false;
	for (int i=1; i<argc; ++i) {
		string arg(argv[i]);
		if (arg == "-i") {
			is_interactive = true;
		} else if (arg == "-v") {
			is_verbose = true;
		} else if (arg == "-h") {
			display_help();
			return 0;
		} else if (is_numeric(arg)) {
			numeric_args.push_back(std::stoi(arg));
		} else if (video_filename.empty()) {
			std::swap(video_filename, arg);
		} else {
			std::swap(csv_filename, arg);
		}
	}
	if (not video_filename.empty() and csv_filename.empty()) {
		csv_filename = replace_extension(video_filename, ".csv");
	}
	if (video_filename.empty()) {
		if (numeric_args.size() >= 1) {
			camera_index = numeric_args[0];
		}
	} else {
		if (numeric_args.size() >= 1) {
			frame_begin = numeric_args[0];
		}
		if (numeric_args.size() >= 2) {
			frame_step = numeric_args[1];
		}
	}
	
    VideoCapture cam = (video_filename.empty()) ? VideoCapture{camera_index} : VideoCapture{video_filename};
    Bitmap3 reference_image;
    if (video_filename.empty()) {
	    for (int i=0; i<10; i++) {
	        assert(reference_image.read(cam));
	    }
	} else {
		for (int i = 0; i < frame_begin; ++i) {
			assert(reference_image.read(cam));
		}
		if (frame_begin == 0) {
			cam = VideoCapture{video_filename};
		}
	}
    try {
        Face state = (is_interactive) ? init_interactive(reference_image) : init_static(reference_image);
        std::cout << " marked " << state() << std::endl;
        set_eye_finder(state);
        if (video_filename.empty()) {
            Pixel size(1650, 1000);
            Gaze fit = calibrate_interactive(state, cam, size);
            track_interactive(state, cam, fit, size);
        } else {
            TrackingData ground_truth = read_csv(csv_filename);
            TrackingData::const_iterator it = ground_truth.begin() + frame_begin;
            Gaze fit = calibrate_static(state, cam, it);
            TrackingData measurement = track_static(state, cam, fit, it);
            printf("average difference %g\n", average_difference(measurement, ground_truth));
        }
    } catch (NoFaceException) {
        std::cerr << "No face initialized." << std::endl;
        return 1;
    }
    return 0;
}
