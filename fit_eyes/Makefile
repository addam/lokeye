CXX = g++
OPTIMIZE = -O2 -ffast-math -march=native
CXXFLAGS = $(OPTIMIZE) -std=c++11 -fopenmp

LIBS = -lopencv_core -lopencv_video -lopencv_videoio -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs
LDFLAGS = $(LIBS) -L/usr/local/lib
OBJS = ui.o image.o main.o optimization.o transformation_locrot.o
BIN = fit_eyes

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

$(OBJS): %.o: %.cpp main.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJS) $(BIN)
