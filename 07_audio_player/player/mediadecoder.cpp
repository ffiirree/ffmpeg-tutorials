#include "mediadecoder.h"
#include "fmt/core.h"
#include "ringbuffer.h"

bool MediaDecoder::open(const std::string& name)
{
    if (running_ || opened_) {
        close();
    }

    // format context
    fmt_ctx_ = avformat_alloc_context();
    if (!fmt_ctx_) {
        return false;
    }

    // open input
    if (avformat_open_input(&fmt_ctx_, name.c_str(), nullptr, nullptr) < 0) {
        LOG(ERROR) << "avformat_open_input";
        return false;
    }

    if (avformat_find_stream_info(fmt_ctx_, nullptr) < 0) {
        LOG(ERROR) << "avformat_find_stream_info";
        return false;
    }

    av_dump_format(fmt_ctx_, 0, name.c_str(), 0);

    // find  audio stream
    audio_stream_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_stream_index_ < 0) {
        LOG(ERROR) << "av_find_best_stream()\n";
        return false;
    }

    //  audio decoder context
    audio_decoder_ = avcodec_find_decoder(fmt_ctx_->streams[audio_stream_index_]->codecpar->codec_id);
    if (!audio_decoder_) {
        LOG(ERROR) << "avcodec_find_decoder() for audio";
        return false;
    }

    audio_decoder_ctx_ = avcodec_alloc_context3(audio_decoder_);
    if (!audio_decoder_ctx_) {
        LOG(ERROR) << "avcodec_alloc_context3 failed for audio";
        return false;
    }

    if (avcodec_parameters_to_context(audio_decoder_ctx_, fmt_ctx_->streams[audio_stream_index_]->codecpar) < 0) {
        LOG(ERROR) << "avcodec_parameters_to_context";
        return false;
    }

    if (avcodec_open2(audio_decoder_ctx_, audio_decoder_, nullptr) < 0) {
        LOG(ERROR) << "avcodec_open2 failed for audio";
        return false;
    }

    LOG(INFO) << fmt::format("[DECODER] SAMPLE_RATE = {}, CHANNELS = {}, CHANNEL_LAYOUT = {}, SAMPLE_FMT = {}, SAMPLE_SIZE = {}",
                             audio_decoder_ctx_->sample_rate, audio_decoder_ctx_->channels,
                             audio_decoder_ctx_->channel_layout,
                             av_get_sample_fmt_name(audio_decoder_ctx_->sample_fmt),
                             av_get_bytes_per_sample(audio_decoder_ctx_->sample_fmt) * 8
    );

    // audio resample
    swr_ctx_ = swr_alloc_set_opts(swr_ctx_,
                                  av_get_default_channel_layout(2),
                                  AV_SAMPLE_FMT_S16,
                                  audio_decoder_ctx_->sample_rate,
                                  av_get_default_channel_layout(audio_decoder_ctx_->channels),
                                  audio_decoder_ctx_->sample_fmt,
                                  audio_decoder_ctx_->sample_rate,
                                  0, nullptr);
    if (!swr_ctx_) {
        LOG(ERROR) << "swr_alloc()";
        return false;
    }

    if (swr_init(swr_ctx_) < 0) {
        LOG(ERROR) << "swr_init()";
        return false;
    }

    // prepare
    packet_ = av_packet_alloc();
    decoded_audio_frame_ = av_frame_alloc();

    opened_ = true;
    LOG(INFO) << "[DECODER]: " << name << " is opened";
    return true;
}

void MediaDecoder::audio_thread_f()
{
    LOG(INFO) << "[AUDIO THREAD] STARTED@" << std::this_thread::get_id();
    defer(LOG(INFO) << "[READ THREAD] EXITED");

    RingBuffer ring_buffer(period_size_ * 2);

    while (running()) {
        av_packet_unref(packet_);
        int ret = av_read_frame(fmt_ctx_, packet_);
        if (ret < 0) {
            if ((ret == AVERROR_EOF || avio_feof(fmt_ctx_->pb))) {
                LOG(INFO) << "[AUDIO THREAD] PUT NULL PACKET TO FLUSH DECODERS";
                // [flushing] 1. Instead of valid input, send NULL to the avcodec_send_packet() (decoding) or avcodec_send_frame() (encoding) functions. This will enter draining mode.
                // [flushing] 2. Call avcodec_receive_frame() (decoding) or avcodec_receive_packet() (encoding) in a loop until AVERROR_EOF is returned.The functions will not return AVERROR(EAGAIN), unless you forgot to enter draining mode.
                av_packet_unref(packet_);
                packet_->stream_index = audio_stream_index_;
            }
            else {
                LOG(ERROR) << "[AUDIO THREAD] read frame failed";
                break;
            }
        }

        first_pts_ = (first_pts_ == AV_NOPTS_VALUE) ? av_gettime_relative() : first_pts_;

        if (packet_->stream_index == audio_stream_index_){
            ret = avcodec_send_packet(audio_decoder_ctx_, packet_);
            while (ret >= 0) {
                av_frame_unref(decoded_audio_frame_);
                ret = avcodec_receive_frame(audio_decoder_ctx_, decoded_audio_frame_);
                if (ret == AVERROR(EAGAIN) ) {
                    break;
                }
                else if (ret == AVERROR_EOF) { // fully flushed, exit
                    avcodec_flush_buffers(audio_decoder_ctx_);
                    LOG(INFO) << "[AUDIO THREAD] EOF";
                    return;
                }
                else if (ret < 0) { // error, exit
                    LOG(ERROR) << "[AUDIO THREAD] legitimate decoding errors";
                    return;
                }

                decoded_audio_frame_->pts = (decoded_audio_frame_->pts == AV_NOPTS_VALUE) ?
                                            av_rescale_q(av_gettime_relative() - first_pts_, { 1, AV_TIME_BASE }, fmt_ctx_->streams[packet_->stream_index]->time_base) :
                                            decoded_audio_frame_->pts - fmt_ctx_->streams[packet_->stream_index]->start_time;

                // decoded frame@{
                auto buffer = (uint8_t *)av_malloc(decoded_audio_frame_->nb_samples * 2 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16));
                int samples_pre_ch = swr_convert(swr_ctx_,
                                                 &buffer, decoded_audio_frame_->nb_samples,
                                                 (const uint8_t**)decoded_audio_frame_->data, decoded_audio_frame_->nb_samples);
                ring_buffer.write((const char *)buffer, samples_pre_ch * 2 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16));
                av_free(buffer);

                while(ring_buffer.size() >= period_size_) {
                    if (ring_buffer.continuous_size() < period_size_) {
                        ring_buffer.defrag();
                    }

                    auto [written_size, ok] = audio_callback_(ring_buffer);
                }

                LOG(INFO) << fmt::format("[AUDIO THREAD] pts = {:>6.3f}s, ts = {:>6.3f}s",
                                         decoded_audio_frame_->pts * av_q2d(fmt_ctx_->streams[audio_stream_index_]->time_base),
                                         (av_gettime_relative() - first_pts_) / 1000000.0
                );
                // @}
            }
        }
    }
}

void MediaDecoder::close()
{
    running_ = false;
    opened_ = false;

    if (audio_thread_.joinable()) audio_thread_.join();

    first_pts_ = AV_NOPTS_VALUE;

    av_packet_free(&packet_);

    av_frame_free(&decoded_audio_frame_);

    avcodec_free_context(&audio_decoder_ctx_);
    avformat_close_input(&fmt_ctx_);

    swr_free(&swr_ctx_);

    LOG(INFO) << "[DECODER] CLOSED";
}
