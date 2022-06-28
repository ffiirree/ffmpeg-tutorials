# Generating GIF

**Graphics Interchange Format**(GIF)是一种支持动画的`bitmap`图片格式。GIF的仅支持最大256种颜色的调色盘(palette)，而常见的图片的是24-bit的RGB格式，GIF需要从24-bit(256 * 256 * 256)种颜色选择8-bit(256)种颜色，所以GIF很容易出现颜色失真、色块等问题。不过GIF支持全局调色盘或者每一帧一个调色盘，这个调色盘的颜色可以修改，所以好的算法可以获取更好的质量。

GIF的另一个问题是，比如和h265编码的mp4文件相比，压缩比较低，也就是文件会非常大，一般来讲，相同算法下，体积越大的GIF质量越高。因此，重要的是在保证质量的情况下，获得更小的体积。

> 注：
>
> 本章节没有代码，代码直接使用complex_filter章节代码即可，修改filter的描述字符串，和以下使用的相同。
> 
> 本章节主要比较不同参数下获得的ffmpeg的效果。
> 
> 不同的视频转为GIF在不同的参数下效果和体积可能差别比较大，参数视情况而定。

## 影响GIF质量和体积的参数

- 分辨率: 分辨率越大，体积越大；除非有要求，一般设置较低的分辨率即可
- 帧率: 帧率越大，体积越大；不很影响画面质量，帧率越低，动作越卡，画面缓慢的可以设定非常低的帧率
- 调色盘: 对画面质量影响非常大，但对体积影响一般没有帧率和分辨率影响大(看实际情况)。高质量GIF必须生成对应的调色盘：
    - 全局/每帧: ffmpeg有一个默认的全局调色盘，是固定的，这是最差的选择；其次可以为要转GIF的视频或图片生成专门的调色盘，这一般是最好的选择；质量最好的选择是为每一帧生成一个调色盘，体积比全局调色盘要大，质量一般要好
    - 颜色抖动: 由于颜色数太少，GIF很容易出现色彩断层和色块，颜色抖动可以缓解这种情况，抖动一般会增加体积
    - 最大颜色数: 颜色越少，色彩断层越严重，体积越小，个人觉得128大多情况下还不错

## 测试

效果运行命令即可查看

```bash
# 以下体积是在 Windows 11 & ffmpeg 4.4.1下测试获得，不同系统和不同版本会有差异

# hevc.mkv: 960x408 9s 1.09MB

# ffmpeg 默认情况下，偏色严重，画面模糊，体积还可以
# 4.76MB
ffmpeg -i hevc.mkv -r 15 -y default.gif

# 这是生成的最高质量的GIF，除了有些因为颜色数太少，抖动造成的模糊，质量非常高，但是体积非常大
# 为每帧生成调色盘，原分辨率，原帧率，开启默认抖动
# ~46M
ffmpeg -i hevc.mkv -vf "[0:v] split [a][b];[a] palettegen=stats_mode=single:max_colors=256 [p];[b][p] paletteuse=new=1" -y high_r24_c256.gif

# 降低一点帧率：24->15，体积减小很多，不过这个体积减小的比例在不同分辨率，不同参数下是不一样的，视实际情况而定
# 只要不觉得卡，帧率可以一直降低
# 27.6MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=15,split [a][b];[a] palettegen=stats_mode=single:max_colors=256 [p];[b][p] paletteuse=new=1" -y high_r15_c256.gif

# 关闭抖动，已经明显有颜色断层了，但画面观感并没有下降
# 抖动算法有很多，这里仅测量默认抖动算法，抖动效果越好的一般生成的GIF体积也越大
# 不一定要在所有情况下都开抖动，抖动会让画面看起来有一层雾；关闭抖动比较通透，但有断层和色块，看个人喜好和实际效果了
# 23.5MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=15,split [a][b];[a] palettegen=stats_mode=single:max_colors=256 [p];[b][p] paletteuse=new=1:dither=none" -y high_r15_c256_nondither.gif

# 使用全局调色盘，体积在该分辨率和帧率下也有明显降低(有些情况下也不会下降很多)，观感差别不大
# 使用全局调色盘需要先过一遍整个视频或所有图片，除了需要实时生成GIF的情况下都可以选择这种生成方法
# 18.8MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=15,split [a][b];[a] palettegen=stats_mode=full:max_colors=256 [p];[b][p] paletteuse" -y high_full_r15_c256.gif

# 降低最大颜色数：256->128，色彩断层比较严重，特别是分辨率较高的情况下
# 低分辨率下，为了进一步降低体积，可以选择降低颜色数
# 18.6MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=15,split [a][b];[a] palettegen=stats_mode=single:max_colors=128 [p];[b][p] paletteuse=new=1:dither=none" -y medium_r15_c128_nondither.gif

# 再次降低最大颜色数：128->64，除非颜色比较单一的场景，否则不建议进一步降低颜色数了
# 13.8MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=15,split [a][b];[a] palettegen=stats_mode=single:max_colors=64 [p];[b][p] paletteuse=new=1:dither=none" -y low_r15_c64_nondither.gif

# 降低分辨率以及帧率，960->640，显示面积为原先的44%
# 5.76MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=10,scale=640:-1:flags=lanczos,split [a][b];[a] palettegen=stats_mode=single:max_colors=128 [p];[b][p] paletteuse=new=1:dither=none" -y medium_640_r10_c128_nondither.gif

# 使用全局调色盘，涂抹感要严重一些
# 3.04MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=10,scale=640:-1:flags=lanczos,split [a][b];[a] palettegen=stats_mode=full:max_colors=128 [p];[b][p] paletteuse=dither=none" -y medium_full_640_r10_c128_nondither.gif

# 继续降低帧率和分辨率，分辨率降低后，色彩断层有了明显改善，且体积也比较令人满意了
# 1.68MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=5,scale=480:-1:flags=lanczos,split [a][b];[a] palettegen=stats_mode=single:max_colors=128 [p];[b][p] paletteuse=new=1:dither=none" -y medium_480_r5_c128_nondither.gif

# 全局调色盘的情况下，256色彩数，感觉这个一般是最好选择了
# 1.39MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=5,scale=480:-1:flags=lanczos,split [a][b];[a] palettegen=stats_mode=full [p];[b][p] paletteuse=dither=none" -y medium_full_480_r5_c256_nondither.gif

# 调色盘生成方式，full->diff，差距不明显，也可选择
# 1.32MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=5,scale=480:-1:flags=lanczos,split [a][b];[a] palettegen=stats_mode=diff [p];[b][p] paletteuse=dither=none" -y medium_diff_480_r5_c256_nondither.gif

# 以下两个再次降低了颜色数，在低分辨率下，不是很影响观感，效果也很好
# 要小体积，可以选择这两个
# 0.99MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=5,scale=480:-1:flags=lanczos,split [a][b];[a] palettegen=stats_mode=full:max_colors=128 [p];[b][p] paletteuse=dither=none" -y medium_full_480_r5_c128_nondither.gif

# 0.98MB
ffmpeg -i hevc.mkv -vf "[0:v] fps=5,scale=480:-1:flags=lanczos,split [a][b];[a] palettegen=stats_mode=diff:max_colors=128 [p];[b][p] paletteuse=new=1:dither=none" -y medium_diff_480_r5_c128_nondither.gif
```

下图是倒数第2条命令的效果：

![GIF](/06_gen_gif/medium_full_480_r5_c128_nondither.gif)

总之，尽量测试几种情况，选择最合适的就可以了。