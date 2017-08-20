
ffmpeg -r 30 -start_number 0 -i frame_%d.ppm -vframes 2000 -c:v libx264 nor_ppm_to_video.mp4
