CXX = g++
OPTIMIZE = -O2 -ffast-math -march=native
CXXFLAGS = $(OPTIMIZE) -std=c++11 -fopenmp

LIBS = -lopencv_core -lopencv_video -lopencv_videoio -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs
# -lceres -lglog
LDFLAGS = $(LIBS) -L/usr/local/lib
OBJ_OPTIMIZATION = optimization.o
OBJS = ui.o bitmap.o main.o $(OBJ_OPTIMIZATION) transformation_locrot.o
BIN = fit_eyes

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

test_bitmap: bitmap.o
test_main: bitmap.o transformation_locrot.o
test_face: bitmap.o transformation_locrot.o $(OBJ_OPTIMIZATION) ui.o
test_eigenface: bitmap.o transformation_locrot.o $(OBJ_OPTIMIZATION) ui.o
test_gaze: bitmap.o transformation_locrot.o $(OBJ_OPTIMIZATION)
test_%: test_%.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

$(OBJS): %.o: %.cpp main.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_ceres optimization_ceres.o: CXXFLAGS += -I/usr/include/eigen3
 
.PHONY: clean

clean:
	rm -f $(OBJS) $(BIN)
