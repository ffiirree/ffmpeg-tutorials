#include <vector>
#include <string>
#include <thread>
#include <atomic>

extern "C" {
#include <libavformat\avformat.h>
#include <libavcodec\avcodec.h>
#include <libavutil\opt.h>
#include <libavutil\time.h>
#include <libavfilter\avfilter.h>
#include <libavfilter\buffersink.h>
#include <libavfilter\buffersrc.h>
}

#include "logging.h"
#include "ringvector.h"
#include "fmt/format.h"

class Decoder {
public:
    Decoder()
    {
        packet_ = av_packet_alloc();
        video_frame_ = av_frame_alloc();
        audio_frame_ = av_frame_alloc();
    }

    ~Decoder()
    {
        avformat_close_input(&fmt_ctx_);

        avcodec_free_context(&video_decode_ctx_);
        avcodec_free_context(&audio_decode_ctx_);

        av_packet_free(&packet_);
        av_frame_free(&video_frame_);
        av_frame_free(&audio_frame_);
    }

    int open(const std::string& filename)
    {
        fmt_ctx_ = avformat_alloc_context();

        if (avformat_open_input(&fmt_ctx_, filename.c_str(), nullptr, nullptr) < 0) {
            LOG(ERROR) << "avformat_open_input()";
            return -1;
        }

        if (avformat_find_stream_info(fmt_ctx_, nullptr) < 0) {
            LOG(ERROR) <<  "avformat_find_stream_info()";
            return -1;
        }

        av_dump_format(fmt_ctx_, 0, filename.c_str(), 0);

        video_stream_idx_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (video_stream_idx_ < 0) {
            LOG(ERROR) <<  "av_find_best_stream()";
            return -1;
        }

        // decoder
        video_decoder_ = avcodec_find_decoder(fmt_ctx_->streams[video_stream_idx_]->codecpar->codec_id);
        if (!video_decoder_) {
            LOG(ERROR) << "avcodec_find_decoder()";
            return -1;
        }

        // decoder context
        video_decode_ctx_ = avcodec_alloc_context3(video_decoder_);
        if(!video_decode_ctx_) {
            fprintf(stderr, "decoder: avcodec_alloc_context3()\n");
            return -1;
        }

        if(avcodec_parameters_to_context(video_decode_ctx_, fmt_ctx_->streams[video_stream_idx_]->codecpar) < 0) {
            fprintf(stderr, "decoder: avcodec_parameters_to_context()\n");
            return -1;
        }

        AVDictionary * decoder_options = nullptr;
        av_dict_set(&decoder_options, "threads", "auto", AV_DICT_DONT_OVERWRITE);
        if(avcodec_open2(video_decode_ctx_, video_decoder_, &decoder_options) < 0) {
            fprintf(stderr, "decoder: avcodec_open2()\n");
            return -1;
        }

        LOG(INFO) <<  fmt::format("[DECODER] {}: {}x{}, framerate = {}/{}, timebase = {}/{}",
                                  filename, video_decode_ctx_->width, video_decode_ctx_->height,
                                  video_decode_ctx_->framerate.num, video_decode_ctx_->framerate.den,
                                  fmt_ctx_->streams[video_stream_idx_]->time_base.num, fmt_ctx_->streams[video_stream_idx_]->time_base.den);

        return 0;
    }

    void decode()
    {
        running_ = true;
        std::thread([this](){ decode_thread(); }).detach();
    }

    void decode_thread()
    {
        LOG(INFO) << "[DECODE THREAD] START @ " << std::this_thread::get_id();

        while(running_) {
            av_packet_unref(packet_);
            int ret = av_read_frame(fmt_ctx_, packet_);
            if (ret < 0) {
                if ((ret == AVERROR_EOF || avio_feof(fmt_ctx_->pb))) {
                    LOG(INFO) << "[READ THREAD] PUT NULL PACKET TO FLUSH DECODERS";
                    av_packet_unref(packet_);
                    break;
                }

                LOG(ERROR) << "[READ THREAD] read frame failed";
                break;
            }

            if (packet_->stream_index == video_stream_idx_) {
                ret = avcodec_send_packet(video_decode_ctx_, packet_);
                while (ret >= 0) {
                    av_frame_unref(video_frame_);
                    ret = avcodec_receive_frame(video_decode_ctx_, video_frame_);
                    if (ret == AVERROR(EAGAIN)) {
                        break;
                    }
                    else if (ret == AVERROR_EOF) { // fully flushed, exit
                        running_ = false;
                        break;
                    }
                    else if (ret < 0) { // error, exit
                        LOG(ERROR) << "[VIDEO THREAD] legitimate decoding errors";
                        running_ = false;
                        break;
                    }

                    while(video_frame_buffer_.full()) {
                        av_usleep(20000);
                    }


                    LOG(INFO) << fmt::format("[DECODER @ {}] pts = {}, frame = {}",
                                             std::hash<std::thread::id>{}(std::this_thread::get_id()), video_frame_->pts, video_decode_ctx_->frame_number);
                    video_frame_buffer_.push([this](AVFrame *frame) {
                        av_frame_unref(frame);
                        av_frame_move_ref(frame, video_frame_);
                    });
                }
            }
            else if(packet_->stream_index == audio_stream_idx_) {
                // TODO:
            }
        }

        LOG(INFO) << "[DECODER] EXIT";
        running_ = false;
    }

    std::string filter_args()
    {
        auto video_stream = fmt_ctx_->streams[video_stream_idx_];
        return fmt::format(
                "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}",
                video_decode_ctx_->width, video_decode_ctx_->height, video_decode_ctx_->pix_fmt,
                video_stream->time_base.num, video_stream->time_base.den,
                video_stream->sample_aspect_ratio.num, video_stream->sample_aspect_ratio.den
        );
    }

//private:
    std::atomic<bool> running_{false};

    int video_stream_idx_{-1};
    int audio_stream_idx_{-1};
    AVFormatContext * fmt_ctx_{nullptr};

    AVCodecContext * video_decode_ctx_{nullptr};
    AVCodecContext * audio_decode_ctx_{nullptr};

    AVCodec * video_decoder_{nullptr};
    AVCodec * audio_decoder_{nullptr};

    AVPacket *packet_{nullptr};
    AVFrame * video_frame_{nullptr};
    AVFrame * audio_frame_{nullptr};

    RingVector<AVFrame*, 10> video_frame_buffer_{
            []() { return av_frame_alloc(); },
            [](AVFrame** frame) { av_frame_free(frame); }
    };

    RingVector<AVFrame*, 10> audio_frame_buffer_{
            []() { return av_frame_alloc(); },
            [](AVFrame** frame) { av_frame_free(frame); }
    };
};

class Encoder{
public:
    Encoder()
    {
        packet_ = av_packet_alloc();
    }
    ~Encoder()
    {
        av_write_trailer(fmt_ctx_);

        avformat_free_context(fmt_ctx_);

        avcodec_free_context(&video_encode_ctx_);
        avcodec_free_context(&audio_encode_ctx_);

        av_packet_free(&packet_);
    }

    int open(const std::string& filename, int w, int h, AVPixelFormat format, AVRational sar, AVRational framerate, AVRational time_base)
    {
        if (avformat_alloc_output_context2(&fmt_ctx_, nullptr, nullptr, filename.c_str()) < 0) {
            LOG(ERROR) << "avformat_alloc_output_context2";
            return -1;
        }

        if(avformat_new_stream(fmt_ctx_, nullptr) == nullptr) {
            LOG(ERROR) << "avformat_new_stream";
            return -1;
        }

        video_encoder_ = avcodec_find_encoder_by_name("libx264");
        if (!video_encoder_) {
            LOG(ERROR) << "avcodec_find_encoder_by_name";
            return -1;
        }
        video_encode_ctx_ = avcodec_alloc_context3(video_encoder_);
        if (!video_encode_ctx_) {
            LOG(ERROR) << "avcodec_alloc_context3";
            return -1;
        }

        AVDictionary* encoder_options = nullptr;
        av_dict_set(&encoder_options, "preset", "ultrafast", AV_DICT_DONT_OVERWRITE);
        av_dict_set(&encoder_options, "tune", "zerolatency", AV_DICT_DONT_OVERWRITE);
        av_dict_set(&encoder_options, "crf", "23", AV_DICT_DONT_OVERWRITE);
        av_dict_set(&encoder_options, "threads", "auto", AV_DICT_DONT_OVERWRITE);

        // encoder codec params
        video_encode_ctx_->height = h;
        video_encode_ctx_->width = w;
        video_encode_ctx_->sample_aspect_ratio = sar;
        video_encode_ctx_->pix_fmt = format;

        // time base
        video_encode_ctx_->time_base = time_base;
        fmt_ctx_->streams[video_stream_idx_]->time_base = time_base;
        video_encode_ctx_->framerate = framerate;

        if (fmt_ctx_->oformat->flags & AVFMT_GLOBALHEADER) {
            video_encode_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        if(avcodec_open2(video_encode_ctx_, video_encoder_, &encoder_options) < 0) {
            LOG(ERROR) << "encoder: avcodec_open2()";
            return -1;
        }

        if(avcodec_parameters_from_context(fmt_ctx_->streams[video_stream_idx_]->codecpar, video_encode_ctx_) < 0) {
            LOG(ERROR) << "encoder: avcodec_parameters_from_context()";
            return -1;
        }

        if(!(fmt_ctx_->oformat->flags & AVFMT_NOFILE)) {
            if (avio_open(&fmt_ctx_->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
                LOG(ERROR) << "avio_open()";
                return -1;
            }
        }

        if(avformat_write_header(fmt_ctx_, nullptr) < 0) {
            LOG(ERROR) << "avformat_write_header()";
            return -1;
        }

        av_dump_format(fmt_ctx_, 0, filename.c_str(), 1);

        av_dict_free(&encoder_options);

        return 0;
    }

    int encode_frame(AVFrame * frame)
    {
        int ret = avcodec_send_frame(video_encode_ctx_, frame);
        while(ret >= 0) {
            av_packet_unref(packet_);
            ret = avcodec_receive_packet(video_encode_ctx_, packet_);

            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if(ret < 0) {
                LOG(ERROR) << "encoder: avcodec_receive_packet()";
                return -1;
            }

            packet_->stream_index = video_stream_idx_;
            LOG(INFO) << fmt::format("[ENCODER] pts = {}, frame = {}", packet_->pts, video_encode_ctx_->frame_number);
            av_packet_rescale_ts(packet_, video_encode_ctx_->time_base, fmt_ctx_->streams[video_stream_idx_]->time_base);

            if (av_interleaved_write_frame(fmt_ctx_, packet_) != 0) {
                LOG(ERROR) << "encoder: av_interleaved_write_frame()";
                return -1;
            }
        }

        return ret;
    }
//private:
    AVFormatContext * fmt_ctx_{nullptr};
    AVCodecContext * video_encode_ctx_{nullptr};
    AVCodecContext * audio_encode_ctx_{nullptr};

    AVCodec * video_encoder_{nullptr};
    AVCodec * audio_encoder_{nullptr};

    int video_stream_idx_ = 0;
    int audio_stream_idx = 1;

    AVPacket * packet_{nullptr};
};

class ComplexFilter {
public:
    ComplexFilter()
    {
        filter_graph_ = avfilter_graph_alloc();
    }

    ~ComplexFilter()
    {
        avfilter_graph_free(&filter_graph_);
    }

    int create_buffersrc(const std::string& args)
    {
        LOG(INFO) << "create buffersrc for: " << args;

        const AVFilter *buffersrc = avfilter_get_by_name("buffer");
        if (!buffersrc) {
            LOG(ERROR) << "avfilter_get_by_name(\"buffer\")";
            return -1;
        }
        AVFilterContext * filter_ctx = nullptr;
        if (avfilter_graph_create_filter(&filter_ctx, buffersrc, "in", args.c_str(), nullptr, filter_graph_) < 0) {
            LOG(ERROR) << "avfilter_graph_create_filter(&filter_ctx)";
            return -1;
        }

        buffersrc_ctxs_.push_back(filter_ctx);
        return 0;
    }

    int create(const std::string& descr)
    {
        const AVFilter *buffersink = avfilter_get_by_name("buffersink");
        if (!buffersink) {
            LOG(ERROR) << "avfilter_get_by_name(\"buffersink\")";
            return -1;
        }
        enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
        if (avfilter_graph_create_filter(&buffersink_ctx_, buffersink, "sink", nullptr, nullptr, filter_graph_) < 0) {
            LOG(ERROR) << "avfilter_graph_create_filter(&out_filter_ctx)";
            return -1;
        }
        if(av_opt_set_int_list(buffersink_ctx_, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) < 0) {
            LOG(ERROR) << "av_opt_set_int_list()";
            return -1;
        }

        AVFilterInOut* inputs = nullptr;
        AVFilterInOut* outputs = nullptr;

        if (avfilter_graph_parse2(filter_graph_, descr.c_str(), &inputs, &outputs) < 0) {
            LOG(ERROR) << "avfilter_graph_parse2()";
            return -1;
        }

        int i = 0;
        for (auto ptr = inputs; ptr; ptr = ptr->next, i++) {
            if (avfilter_link(buffersrc_ctxs_[i], 0, ptr->filter_ctx, ptr->pad_idx) != 0) {
                LOG(ERROR) << "avfilter_link()";
                return -1;
            }
        }

        for (auto ptr = outputs; ptr; ptr = ptr->next) {
            if (avfilter_link(ptr->filter_ctx, ptr->pad_idx, buffersink_ctx_, 0) != 0) {
                LOG(ERROR) << "avfilter_link()";
                return -1;
            }
        }

        if(avfilter_graph_config(filter_graph_, nullptr) < 0) {
            LOG(ERROR) << "avfilter_graph_config()";
            return -1;
        }

        char * graph = avfilter_graph_dump(filter_graph_, nullptr);
        if (graph) {
            LOG(INFO) << "graph >>>> \n%s\n" << graph;
        } else {
            LOG(ERROR) << "avfilter_graph_dump()";
            return -1;
        }

        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);

        return 0;
    }

    AVRational time_base() { return av_buffersink_get_time_base(buffersink_ctx_); }
    AVRational sample_aspect_ratio() { return av_buffersink_get_sample_aspect_ratio(buffersink_ctx_); }
    int height() { return av_buffersink_get_h(buffersink_ctx_); }
    int width() { return av_buffersink_get_w(buffersink_ctx_); }
    AVRational framerate() { return av_buffersink_get_frame_rate(buffersink_ctx_); }
    AVPixelFormat format() { return (AVPixelFormat)av_buffersink_get_format(buffersink_ctx_); }

    //private:
    std::atomic<bool> running_{false};

    AVFilterGraph * filter_graph_{nullptr};
    std::vector<AVFilterContext*> buffersrc_ctxs_{};
    AVFilterContext* buffersink_ctx_{ nullptr };
};


int main(int argc, char* argv[])
{
    if (argc < 4) {
        LOG(ERROR) << "complex_filter -i <input-1> -i <input-2> <output>";
        return -1;
    }

    std::vector<std::string> input_files;
    std::string output_file;

    std::vector<std::shared_ptr<Decoder>> decoders;
    ComplexFilter filter;
    Encoder encoder;

    for (int i = 1; i < argc; i++){
        if (std::strcmp("-i", argv[i]) == 0 && i + 1 < argc && argv[i+1][0] != '-') {
            input_files.emplace_back(argv[i+1]);
            i++;
        }
        else if (output_file.empty()){
            output_file = argv[i];
        }
        else {
            LOG(ERROR) << "complex_filter -i <input-1> -i <input-2> <output>";
            return -1;
        }
    }

    for(auto& input: input_files) {
        auto decoder = std::make_shared<Decoder>();
        if(decoder->open(input) < 0) {
            return -1;
        }
        decoders.push_back(decoder);

        filter.create_buffersrc(decoder->filter_args());
    }

    filter.create("[0:v] scale=320:-1:flags=lanczos,vflip [s];[1:v][s]overlay=10:10");
    encoder.open(output_file, filter.width(), filter.height(), filter.format(), filter.sample_aspect_ratio(), filter.framerate(), filter.time_base());

    AVFrame * frame = av_frame_alloc();
    AVFrame * filtered_frame = av_frame_alloc();

    for (auto& decoder : decoders) {
        decoder->decode();
    }

    std::atomic<bool> running{true};
    std::thread([&](){
        LOG(INFO) << "[FILTER THREAD] START @ " << std::this_thread::get_id();

        while(running) {
            for(int i = 0; i < decoders.size(); i++) {

//                running = false;
//                for (auto decoder : decoders) {
//                    if(!decoder->video_frame_buffer_.empty() || decoder->running_) {
//                        running = true;
//                    }
//                }

                // TODO:
                if (decoders[i]->video_frame_buffer_.empty()) {
                    if(decoders[i]->running_) {
                        av_usleep(10000);
                        continue;
                    }
//                    continue;
                }

                int ret = 0;
                if (decoders[i]->video_frame_buffer_.empty()) {
//                    LOG(INFO) << i << " -> null";
                    ret = av_buffersrc_add_frame_flags(filter.buffersrc_ctxs_[i], nullptr, AV_BUFFERSRC_FLAG_PUSH);
//                    av_frame_free(&f);
                }
                else {
                    decoders[i]->video_frame_buffer_.pop([=](AVFrame * popped) {
                        av_frame_unref(frame);
                        av_frame_move_ref(frame, popped);
                    });

                    ret = av_buffersrc_add_frame_flags(filter.buffersrc_ctxs_[i], frame, AV_BUFFERSRC_FLAG_PUSH);
                }
                if(ret < 0) {
                    LOG(ERROR) << "av_buffersrc_add_frame_flags()";
                    running = false;
                    break;
                }
                while(ret >= 0) {
                    av_frame_unref(filtered_frame);
                    ret = av_buffersink_get_frame_flags(filter.buffersink_ctx_, filtered_frame, AV_BUFFERSINK_FLAG_NO_REQUEST);

                    if (ret == AVERROR(EAGAIN)) {
                        break;
                    }
                    else if(ret == AVERROR_EOF) {
                        LOG(INFO) << "[FILTER THREAD] EOF";

                        // flush and exit
                        ret = encoder.encode_frame(nullptr);
                        if (ret == AVERROR_EOF) {
                            running = false;
                        }
                        break;
                    }
                    if (ret < 0) {
                        LOG(ERROR) << "av_buffersink_get_frame_flags()";
                        running = false;
                        break;
                    }

                    ret = encoder.encode_frame(filtered_frame);

                }
            }
        }
        LOG(INFO) << "[FILTER THREAD] EXIT";
        return 0;
    }).join();

    LOG(INFO) << "EXITED";

    av_frame_free(&filtered_frame);
    av_frame_free(&frame);

    return 0;
}