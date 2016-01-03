CXX = g++
CXXFLAGS = -O3 -ffast-math -std=c++11 -fopenmp

LIBS = -lopencv_core -lopencv_video -lopencv_videoio -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs
LDFLAGS = $(LIBS) -L/usr/local/lib
TARGETS = label example_segment_background example_optflow transform

all: $(TARGETS)

$(TARGETS): %: %.cpp
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) -o $@
