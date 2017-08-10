CXX = g++
OPTIMIZE = -O2 -ffast-math -march=native
CXXFLAGS = $(OPTIMIZE) -std=c++11 -fopenmp -Werror=return-type -g
LDFLAGS = $(LIBS) -L/usr/local/lib
TRANSFORMATION = affine
CHILDREN = markers
OBJ_TRANSFORMATION = transformation_$(TRANSFORMATION).o
OBJ_CHILDREN = children_$(CHILDREN).o
OBJ_OPTIMIZATION = optimization.o
CXXFLAGS += -DTSF_HEADER=transformation_$(TRANSFORMATION).h -DCHL_HEADER=children_$(CHILDREN).h

LIBS = -lopencv_core -lopencv_video -lopencv_videoio -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -lopencv_objdetect
OBJS = ui.o bitmap.o $(OBJ_OPTIMIZATION) $(OBJ_TRANSFORMATION) $(OBJ_CHILDREN) eye.o
ALL_OBJS = $(OBJS) children_grid.o children_markers.o
BIN = fit_eyes

all: $(BIN)

$(BIN): $(OBJS) main.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

test_bitmap: bitmap.o
test_main: bitmap.o $(OBJ_TRANSFORMATION)
test_face: ui.o bitmap.o $(OBJ_OPTIMIZATION) $(OBJ_TRANSFORMATION) $(OBJ_CHILDREN) eye.o
test_eigenface: bitmap.o $(OBJ_TRANSFORMATION) $(OBJ_OPTIMIZATION) ui.o
test_gaze: bitmap.o $(OBJ_TRANSFORMATION) $(OBJ_OPTIMIZATION)
test_grid_calibrate: bitmap.o $(OBJ_TRANSFORMATION) $(OBJ_OPTIMIZATION) ui.o
test_transformation: bitmap.o $(OBJ_TRANSFORMATION) $(OBJ_OPTIMIZATION) $(OBJ_CHILDREN)
test_transformation_barycentric: bitmap.o $(OBJ_TRANSFORMATION)
test_optimization: bitmap.o $(OBJ_TRANSFORMATION) $(OBJ_OPTIMIZATION)
cest_ceres: LIBS += -lceres -lglog
test_%: test_%.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

rig_eye: bitmap.o eye.o
rig_face: bitmap.o optimization.o $(OBJ_TRANSFORMATION) $(OBJ_CHILDREN) ui.o
rig_%: rig_%.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

$(OBJS): %.o: %.cpp main.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_ceres optimization_ceres.o: CXXFLAGS += -I/usr/include/eigen3
 
.PHONY: clean

clean:
	rm -f $(ALL_OBJS) $(BIN)
