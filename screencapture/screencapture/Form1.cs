using System;
using System.Windows.Forms;

namespace screencapture
{
    public partial class Form1 : Form
    {
        private VideoRecorder recorder;
        bool recording = false;

        public Form1()
        {
            recorder = new VideoRecorder();
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (recording) {
                button1.Text = "Record";
                recording = false;
                recorder.EndRecording();
            } else { 
                SaveFileDialog sfd = new SaveFileDialog();
                sfd.Filter = "Video Files (*.avi)|*.avi*";
                sfd.FilterIndex = 1;
                sfd.DefaultExt = "avi";
                sfd.RestoreDirectory = true;
                if (sfd.ShowDialog() == DialogResult.OK) {
                    button1.Text = "Stop";
                    recording = true;
                    recorder.StartRecording(sfd.FileName.ToString());
                }
            }
        }
    }
}
