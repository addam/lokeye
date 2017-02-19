using System;
using System.Collections.Generic;
using AForge.Video.FFMPEG;
using System.Drawing;
using EyeXFramework;
using Tobii.EyeX.Framework;
using CsvHelper;
using System.IO;
using System.Globalization;
using AForge.Video;
using AForge.Video.DirectShow;
using System.Threading;

namespace screencapture
{
    class Vector
    {
        public double X, Y;
        public Vector(double x, double y)
        {
            X = x;
            Y = y;
        }
        public static Vector operator+ (Vector self, GazePointEventArgs other)
        {
            return new Vector(self.X + other.X, self.Y + other.Y);
        }
        public static Vector operator/ (Vector self, double other)
        {
            return new Vector(self.X / other, self.Y / other);
        }
    }
    class VideoRecorder
    {
        String filename;
        private VideoFileWriter videoWriter;
        private Mutex recording = new Mutex(true);
        List<GazePointEventArgs> gazeRecord = new List<GazePointEventArgs>();
        EyeXHost _eyeXHost;
        GazePointDataStream _lightlyFilteredGazeDataStream;
        private TextWriter textWriter;
        private CsvWriter csv;
        private CultureInfo usCulture = new CultureInfo("en-US");
        private VideoCaptureDevice videoSource;

        public void StartRecording(string filenameToSave)
        {
            filename = filenameToSave;
            textWriter = File.CreateText(Path.ChangeExtension(filenameToSave, ".csv"));
            csv = new CsvWriter(textWriter);
            csv.Configuration.CultureInfo = usCulture;

            var videoDevices = new FilterInfoCollection(FilterCategory.VideoInputDevice);
            videoSource = new VideoCaptureDevice(videoDevices[0].MonikerString);
            videoSource.NewFrame += new NewFrameEventHandler(video_NewFrame);

            videoWriter = new VideoFileWriter();

            _eyeXHost = new EyeXHost();
            _lightlyFilteredGazeDataStream = _eyeXHost.CreateGazePointDataStream(GazePointDataMode.LightlyFiltered);
            _eyeXHost.Start();
            _lightlyFilteredGazeDataStream.Next += gazeDataStreamNext;

            videoSource.Start();
            recording.ReleaseMutex();
        }

        public void EndRecording()
        {
            videoSource.SignalToStop();
            recording.WaitOne();
            videoWriter.Close();
            textWriter.Close();
            _lightlyFilteredGazeDataStream.Dispose();
            _eyeXHost.Dispose();
        }

        private void gazeDataStreamNext(object s, GazePointEventArgs e)
        {
            gazeRecord.Add(e);
        }

        private void video_NewFrame(object sender, NewFrameEventArgs eventArgs)
        {
            if (gazeRecord.Count > 0) {
                Vector average = new Vector(0, 0);
                foreach (var ev in gazeRecord) {
                    average += ev;
                }
                average /= gazeRecord.Count;

                if (recording.WaitOne(0)) {
                    Bitmap bitmap = eventArgs.Frame;
                    if (!videoWriter.IsOpen) {
                        videoWriter.Open(filename, bitmap.Width, bitmap.Height, 25, VideoCodec.MPEG4, 8000000);
                    }
                    videoWriter.WriteVideoFrame(bitmap);
                    csv.WriteField(average.X);
                    csv.WriteField(average.Y);
                    csv.NextRecord();
                    recording.ReleaseMutex();
                }
            }
            gazeRecord.Clear();
        }
    }
}

