# Dynamic Wallpaper
A small console utility that allows you to play video on your wallpaper. It can play a lot of video formats, including most popular: mp4, avi, webm, mkv.
It supports multi-monitors systems and can play video on each monitor. If video's resolution is different than monitor resolution, it can be scaled.

# Build
- Clone or download this repository
- Download and compile **FFmpeg SDK** library from https://ffmpeg.org/ or [FFmpeg githubmirror](https://github.com/FFmpeg/FFmpeg)
- Required **FFmpeg** modules:
    - avcodec
    - avdevice
    - avformat
    - avutil
    - swresample
    - swscale
- Add needed headers and static libraries to your project
- In order to run application you need to have **FFmpeg** dlls in execution folder

# Example
![Picture example](/example/screen_example.png)

[Video example](https://www.youtube.com/watch?v=Tv0yiUbZq2k)