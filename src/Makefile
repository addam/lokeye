CXX = g++
OPTIMIZE = -O2 -ffast-math -march=native
CXXFLAGS = $(OPTIMIZE) -std=c++11 -fopenmp -Werror=return-type -g
LDFLAGS = $(LIBS) -L/usr/local/lib
TRANSFORMATION = affine
OBJ_TRANSFORMATION = transformation_$(TRANSFORMATION).o
OBJ_OPTIMIZATION = optimization.o
CXXFLAGS += -DTSF_HEADER=transformation_$(TRANSFORMATION).h

LIBS = -lopencv_core -lopencv_video -lopencv_videoio -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs
# -lceres -lglog
OBJS = ui.o bitmap.o main.o $(OBJ_OPTIMIZATION) $(OBJ_TRANSFORMATION)
BIN = fit_eyes

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

test_bitmap: bitmap.o
test_main: bitmap.o $(OBJ_TRANSFORMATION)
test_face: bitmap.o $(OBJ_TRANSFORMATION) $(OBJ_OPTIMIZATION) ui.o
test_eigenface: bitmap.o $(OBJ_TRANSFORMATION) $(OBJ_OPTIMIZATION) ui.o
test_gaze: bitmap.o $(OBJ_TRANSFORMATION) $(OBJ_OPTIMIZATION)
test_grid_calibrate: bitmap.o $(OBJ_TRANSFORMATION) $(OBJ_OPTIMIZATION) ui.o
test_transformation: bitmap.o $(OBJ_TRANSFORMATION) $(OBJ_OPTIMIZATION)
test_transformation_barycentric: bitmap.o $(OBJ_TRANSFORMATION)
test_optimization: bitmap.o $(OBJ_TRANSFORMATION) $(OBJ_OPTIMIZATION)
test_%: test_%.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

$(OBJS): %.o: %.cpp main.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_ceres optimization_ceres.o: CXXFLAGS += -I/usr/include/eigen3
 
.PHONY: clean

clean:
	rm -f $(OBJS) $(BIN)
