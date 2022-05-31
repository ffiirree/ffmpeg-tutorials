# Streaming

视频直播已经非常普遍了，推流和拉流是两个基本的技术步骤。

## 推流

将视频/音频流推送到服务器，常见的推流协议为`RTMP`。

## 拉流

从服务器将视频/音频流拉取到本地播放，常见的协议有`RTMP, HLS, HTTP-FLV(HDL)`等。

## 协议

### RTMP

**Real Time Messaging Protocol(RTMP)**, 即时消息传送协议，该协议针对的是Flash Video，即`FLV`。

推流常用`RTMP`。但是拉流由于浏览器等不再支持`flush`，所以拉流受限。

### HLS

**HTTP Live Streaming(HLS)**, 这是 Apple 提出的直播流协议。跨平台性比较好，HTML5可以直接打开播放，移动端兼容性良，就是延迟比较高。

### HTTP-FLV

同样针对于`FLV`视频格式，

## 流媒体服务器搭建

这不是本例子的重点，所以只是搭建一个简单的流媒体服务器用来测试拉流和推流效果。

> 推荐在Ubuntu或WSL2下进行搭建

### Ubuntu/WSL2安装

安装`nginx`和`rtmp`模块

```bash
sudo apt install nginx libnginx-mod-rtmp
```

使用`sudo systemctl status nginx`或浏览器打开`localhost`查看nginx是否运行成功。

### Windows安装

下载`nginx`和`rtmp`对应的包，安装。


### 配置

这里仅做`RTMP`的配置，`HLS`等协议不再介绍，这不是要介绍的重点。

打开nginx配置文件
```bash
sudo vim /etc/nginx/nginx.conf
```

追加内容

```
rtmp {
    server {
        listen 1935;
        chunk_size 4096;

        application live {
                live on;
                record off;
                allow play all;
        }
    }
}
```
测试并重启nginx

```bash
sudo nginx -t
sudo nginx -s reload
```

### 测试

```bash
# 推流
ffmpeg -re -i hevc.mkv -vcodec libx264 -acodec aac -f flv rtmp://127.0.0.1:1935/live/test

# 拉流
ffplay rtmp://127.0.0.1:1935/live/test
```

效果如下

![streaming](/images/streaming.png)


## Hard wary

### 推流

推流过程和转码过程是几乎相同的(如果视频源的编码和封装格式符合条件，可以直接解封装后直接发送，不需要转码等步骤)，这里仅说明几处不同的地方。

创建输出`AVForamtContex`时，选择`flv`格式
```c
CHECK(avformat_alloc_output_context2(&encoder_fmt_ctx, nullptr, "flv", nullptr) >= 0);
```
其次，如果是推送视频文件(非摄像头等)，推流前要使用`av_usleep(sleep_us)`控制推流速度，否则会不间断的推送帧。

### 拉流

直接使用[Player](/09_media_player/README.md)代码就可以了。