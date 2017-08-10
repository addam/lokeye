# lokeye
Lokeye is an eye tracker designed for use with customer grade web cameras (or similar).
It is supposed to help people interact with their computer in a more comfortable manner.

Although this program is the cornerstone of my diploma thesis published in July 2017 it is not complete yet, nor friendly to the end user.

## dependencies
The program requires the OpenCV 3 library.
Build requires GNU Make.
Note that the source code is written in the C++11 standard, and that we use the non-standard keywords `and`, `or` instead of the operators `&&`, `||`.

## installation
In order to build the main program `fit_eyes`, just execute `make` in the root directory.

In order to build a testing program, such as `test_face`, go to the `src/` subdirectory and specify that program as the Make target.

## usage
The program `fit_eyes` will try to localize a human face in the image from the camera.
If this fails, the program throws an error message and stops.

You can mark the face manually if you specify the `-i` option on the command line.
You have to mark firstly the parent tracker over the whole face, and then two eyes by dragging two circles from the center.
For a good tracking it is important that you mark the eyes with their correct radius.

Once a face is detected, the program proceeds with a calibration sequence.
It shows a moving dot on the screen.
Please, watch this dot carefully until it disappears (it should be no longer than 30 seconds).

Finally, a gray window appears with a yellow jagged line.
The line estimates your on-screen gaze position.
You can rejoice the results or press Escape to quit the program.

## configuration

There are four ''motion models'' available for face tracking.
At compile time, you have to set `TRANSFORMATION=<model>` to one of the following options:
 * `locrot`: Location and rotation. Very naive.
 * `affine`: Affine transformation. Flexible quite enough. Default option.
 * `barycentric`: Triangle-based affine transformation. Compared to the previous one, this is somewhat slower and allows you to use Grid Children.
 * `perspective`: Quadrangle-based homography. The most general transformation that still makes sense. Considerably slower than the others.
 
There are two ''children schemes'' for face tracking.
These are switched by the `CHILDREN=<scheme>` directive at compile time:
 * `markers`: Several small markers are set to track interesting facial features. Default option.
 * `grid`: The face area is seamlessly subdivided into several trackers, each responsible of its cut out cell.

After changing these build options, it is necessary to do a `make clean`.

## testing programs

### test_face
Similar to the main program, this programs starts with either autodetection or interactive marking of the face.
Then, it skips the calibration procedure and just shows the interactive face and eye tracking results.
In this program, gaze is not estimated.

### rig_eye
Reads a CSV annotation file from standard input, reads the corresponding images from disk and refines the eye location in each of them.
The input format is `filename,x,y,radius[,anything]`; the filename is directly copied to the output.
The supplied coordinates are understood as ground truth, and each of the algorithms are executed several times from a random nearby location to find it.
Each of these trials produces an output row `(space)algorithm,dx,dy`, where `dx` and `dy` are resulting coordinates relatively to the true center.
All coordinates are expressed in pixel units, from top-left corner.

### test_gaze
Unit test for ransac-based homography fitting.
The results have to be inspected by a human to check that they make sense.
Firstly, a random homography and a set of noisy point correspondences are generated; these should have almost zero fitting error.
Then, the program produces several completely random point sets.
For seven points correspondences or less, the fit should be perfect; then, the average error is quite random.
The median error should remain quite low as long as there are less than fifteen points.

### test_transformation
Unit test for analytical derivatives and other calculations related to the motion models.
In the output, ''analytic derivative'' and ''analytic scale'' should be almost equal to their numeric counterparts.

## tobii recording suite

This program may be useful to many developers and scientists who use Tobii EyeX.
It is a minimalistic application that lets you record the user (on video) and their gaze position, simultaneously.
The output file format is CSV, each row corresponding to a frame of the video, and written down as `x,y` coordinates.

The whole Visual C# 2015 project is located in the `screencapture` directory.
The Tobii Framework is so far available only for Windows.
