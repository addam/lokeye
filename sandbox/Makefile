CXX = g++
CXXFLAGS = -O2 -ffast-math -std=c++11 -fopenmp -march=native

LIBS = -lopencv_core -lopencv_video -lopencv_videoio -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -lboost_filesystem -lboost_system
LDFLAGS = $(LIBS) -L/usr/local/lib
TARGETS = label example_segment_background example_optflow transform eye_hough eye_canny eye_hough2 eye_hough3 eye_hough4 eye_corr eye_gradcorr eye_gradcorr_ellipse eye_iris eye_limbus eye_limbus2 eye_grad_newton synchronization mirror draw_cc

all: eye_hough eye_canny eye_hough2 eye_hough3 eye_corr eye_gradcorr eye_iris eye_limbus eye_limbus2 eye_grad_newton

$(TARGETS): %: %.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

eye_hough2: eye_hough2.cpp ../src/bitmap.o
eye_hough3: eye_hough3.cpp ../src/bitmap.o
