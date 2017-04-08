CXX = g++
CXXFLAGS = -O2 -ffast-math -std=c++11 -fopenmp

LIBS = -lopencv_core -lopencv_video -lopencv_videoio -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs
LDFLAGS = $(LIBS) -L/usr/local/lib
TARGETS = label example_segment_background example_optflow transform eye_hough eye_canny eye_hough2 eye_hough3 eye_hough4 eye_corr eye_gradcorr eye_gradcorr_ellipse eye_iris

all: $(TARGETS)

$(TARGETS): %: %.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

eye_hough%: eye_hough%.cpp ../src/bitmap.o
