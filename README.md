# lokeye
Lokeye is an eye tracker designed for use with customer grade web cameras (or similar).
It is supposed to help people interact with their computer in a more comfortable manner.

Although this program is the cornerstone of my diploma thesis published in July 2017 it is not complete yet, nor friendly to the end user.

## installation
When OpenCV 3.0 and all its dependencies are installed, `make fit_eyes` should build the main program.
If you step into the `src/` directory, you can build and try several smaller targets such as `test_face`, `rig_eye` etc.

## usage
The program `./fit_eyes` will try to localize a human face in the image from the camera.
If this fails, the program throws an error message and stops.

If a face is detected, the program proceeds with a calibration sequence.
It shows a moving dot on the screen.
Please, watch this dot carefully until it disappears (it should be no longer than 30 seconds).

Finally, a gray window appears with a yellow jagged line.
The line estimates your on-screen gaze position.
You can rejoice the results or press Escape to quit the program.

## configuration

There are four 'motion models' available for face tracking.
At compile time, you have to set `TRANSFORMATION=<model>` to one of the following options:
 * `locrot`: Location and rotation. Very naive.
 * `affine`: Affine transformation. Flexible quite enough. Default option.
 * `barycentric`: Triangle-based affine transformation. Compared to the previous one, this is somewhat slower and allows you to use Grid Children.
 * `perspective`: Quadrangle-based homography. Most general transformation that makes sense. Same performance as the previous one, just a bit more buggy.
 
There are two 'children schemes' for face tracking.
These are switched by the `CHILDREN=<scheme>` directive at compile time:
 * `markers`: Several small markers are set to track interesting facial features. Default option.
 * `grid`: The face area is seamlessly subdivided into several trackers, each responsible of its cut out cell. The code is not finished yet.

