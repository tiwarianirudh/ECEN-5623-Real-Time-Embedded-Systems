# ECEN5623
## Real-Time Embedded Systems
## Final Project: Time-Lapse Image Acquisition using POSIX Threads
### Dev-kits and Tools: NVIDIA Jetson-TK1 and Logitech C200 web-cam
### Authors: *Anirudh Tiwari* , *Mukund M Atre*

**Goal**
Design and analyze Time Lapse Real-Time Image acquisition using openCV libraries on RTLinux environment using NVIDIA Jetson-TK1 development board.

>Developed a Real-Time POSIX based Multi-threaded system using Rate Monotonic Scheduling Policy.

>Capturing images at various rates (1Hz,3Hz and 7Hz) with maximum attainable accuracy and minimum latency message queue implementation for storing the captured images.

>Implemented different Image-processing techniques such as background-elimination, sharpening of image, motion detection and compression by analysis.

>Compression of images captured in PPM (portable pixmap) to JPEG.

>Conversion of the captured images to a time lapse MPEG4 video using FFmpeg.

**References**
>a) http://docs.opencv.org for video capture and compression.

>b) https://stackoverflow.com/ for background elimination and c++ reference.

>c) http://opencv-help.blogspot.com for sharpening filter.

>d) https://en.wikipedia.org/ for theory and concepts.

>e) https://stackoverflow.com/questions/27035672/cv-extract-differencesbetween-two-images for reading and understanding the working of foreground detection.

>f) Linux manual pages.

>g) eLinux.org for queries regarding Jetson TK1.

>h) Ffmpeg.org.

>i) RTES example codes provided by Dr. Sam Siewert.

>j) Logitech C200 reference.
