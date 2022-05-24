# Generating GIF

```bash
# hevc.mkv: 960x408 9s 1.09MB

# 4.76MB
ffmpeg -i hevc.mkv -r 15 default.gif

# 27.6MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=15,split [a][b];[a] palettegen=stats_mode=single:max_colors=256 [p];[b][p] paletteuse=new=1" -y high_r15_c256.gif

# 23.5MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=15,split [a][b];[a] palettegen=stats_mode=single:max_colors=256 [p];[b][p] paletteuse=new=1:dither=none" -y high_r15_c256_nondither.gif

# 18.6MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=15,split [a][b];[a] palettegen=stats_mode=single:max_colors=128 [p];[b][p] paletteuse=new=1:dither=none" -y medium_r15_c128_nondither.gif

# 13.8MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=15,split [a][b];[a] palettegen=stats_mode=single:max_colors=64 [p];[b][p] paletteuse=new=1:dither=none" -y low_r15_c64_nondither.gif

# 5.76MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=10,scale=640:-1:flags=lanczos,split [a][b];[a] palettegen=stats_mode=single:max_colors=128 [p];[b][p] paletteuse=new=1:dither=none" medium_640_r10_c128_nondither.gif

# 1.68MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=10,scale=320:-1:flags=lanczos,split [a][b];[a] palettegen=stats_mode=single:max_colors=128 [p];[b][p] paletteuse=new=1:dither=none" medium_320_r10_c128_nondither.gif

```