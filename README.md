# lokeye
Lokeye is an eye tracker designed for use with customer grade web cameras (or similar).
It is supposed to help people interact with their computer in a more comfortable manner.

The program is yet in early development, there are no usable binaries.
Moreover, it is the cornerstone of my diploma thesis meant for publishing in summer 2017, so please show a bit of scientific ethics when reusing the code.

## important files
* `src/test_face.cpp` is a real-time face and eye tracker. Initialization is done by hand, marking the face area and then both pupils.
* `src/main.cpp` is a real-time gaze tracker and the main file of this whole project. After marking the face and pupils, a calibration session starts. Upon calibration, eye movement is visualized on-screen.

The following files are old and no longer maintained:
* `sandbox/transform.cpp` can visualize and rotate a depth-mapped photograph in camera projection.
  It can also evaluate an error metric against reference photo and should minimize this error automatically.
* `sandbox/label.cpp` performs a local fitting to circular edges in a photograph.
  With the help of a human, it can localize the pupil with a high precision.
  Testing photographs can be downloaded from the `lokeye-data` repository.
* `sandbox/fitrow.cpp` fits one depth map onto another via a 3D isometric transformation.
  The optimization is unstable and slow.

## installation
When OpenCV 3.0 and all its dependencies are installed, `make transform` and `make label` should build the respective binaries.
Since the tools are experimental, they are not very friendly to the user.
