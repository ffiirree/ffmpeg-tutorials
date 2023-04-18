# Hardware Acceleration

- [ ] encoding
- [x] decoding

```bash
# CUDA
ffmpeg -hwaccel cuda -hwaccel_output_format cuda -c:v h264_cuvid -i h264.mkv -c:a copy -c:v h264_nvenc hwh264.mp4
```

## References

[hw_decode.c](https://ffmpeg.org/doxygen/trunk/hw_decode_8c-example.html)