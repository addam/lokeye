# lokeye
Lokeye is an eye tracker designed for use with customer grade web cameras (or similar).
It is supposed to help people interact with their computer more comfortably.

The program is yet in early development, there are no usable binaries.
Moreover, it is the cornerstone of my diploma thesis meant for publishing in summer 2016, so please show a bit of scientific ethics when reusing the code.

## important files
* `transform.cpp` can visualize and rotate a depth-mapped photograph in camera projection.
  It can also evaluate an error metric against reference photo and should minimize this error automatically.
* `label.cpp` performs a local fitting to circular edges in a photograph.
  With the help of a human, it can localize the pupil with high precision
Testing photographs are not included in the repository (for now) because they take up over 300M.

## installation
When OpenCV 3.0 and all its dependencies are installed, `make transform` and `make label` should build the respective binaries.
Since the tools are experimental, they are not very friendly to the user.
