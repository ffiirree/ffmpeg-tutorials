# Complex filters

```bash
ffmpeg -i hevc.mkv -filter_complex "[0:v]split[v1][v2];[v1]scale=320:-1:flags=lanczos,vflip[s];[v2][s]overlay=10:10" -c:v libx264 x264.mp4
```