# Recording

## Windows
```bash
# 列出所有设备
ffmpeg -hide_banner -f dshow -list_devices true -i dummy

# 录制摄像头
ffmpeg -f dshow -i video="HD WebCam" -c:v libx264 camera.mp4
# 录制屏幕
ffmpeg -f gdigrab -framerate 25 -offset_x 100 -offset_y 200 -video_size 720x360 -i desktop -c:v libx264 screen.mp4
```

## Linux

```bash
# 录制摄像头
ffmpeg -f v4l2 -i /dev/video0 -c:v libx264 camera.mkv
# 录制屏幕
ffmpeg -framerate 25 -video_size 720x360 -f x11grab -i :0.0+100,200 -c:v libx264 screen.mp4
```