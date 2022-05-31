#include "mediadecoder.h"
#include "fmt/core.h"
#include "fmt/ranges.h"

bool MediaDecoder::open(const std::string& name, const std::string& format, const string& filters_descr, AVPixelFormat pix_fmt, const std::map<std::string, std::string>& options)
{
	pix_fmt_ = pix_fmt;
	filters_descr_ = filters_descr;

	LOG(INFO) << fmt::format("[DECODER] filters = \"{}\", options = {}", filters_descr, options);

    if (running_ || opened_) {
        close();
    }

	// format context
	fmt_ctx_ = avformat_alloc_context();
	if (!fmt_ctx_) {
		return false;
	}

	avdevice_register_all();

	// input format
	AVInputFormat* input_fmt = nullptr;
	if (!format.empty()) {
		input_fmt = av_find_input_format(format.c_str());
		if (!input_fmt) {
			LOG(ERROR) << "av_find_input_format";
			return false;
		}
	}

	// options
	AVDictionary* input_options = nullptr;
	defer(av_dict_free(&input_options));
	for (const auto& [key, value] : options) {
		av_dict_set(&input_options, key.c_str(), value.c_str(), 0);
	}

	// open input
	if (avformat_open_input(&fmt_ctx_, name.c_str(), input_fmt, &input_options) < 0) {
		LOG(ERROR) << "avformat_open_input";
		return false;
	}

	if (avformat_find_stream_info(fmt_ctx_, nullptr) < 0) {
		LOG(ERROR) << "avformat_find_stream_info";
		return false;
	}

	av_dump_format(fmt_ctx_, 0, name.c_str(), 0);

	// find video & audio stream
    video_stream_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	if (video_stream_index_ < 0) {
		LOG(ERROR) << "av_find_best_stream()\n";
		return false;
	}

	// decoder
    video_decoder_ = avcodec_find_decoder(fmt_ctx_->streams[video_stream_index_]->codecpar->codec_id);
    if (!video_decoder_) {
        LOG(ERROR) << "avcodec_find_decoder() for video";
        return false;
    }

    // video decoder context
    video_decoder_ctx_ = avcodec_alloc_context3(video_decoder_);
    if (!video_decoder_ctx_) {
        LOG(ERROR) << "avcodec_alloc_context3";
        return false;
    }

    if (avcodec_parameters_to_context(video_decoder_ctx_, fmt_ctx_->streams[video_stream_index_]->codecpar) < 0) {
        LOG(ERROR) << "avcodec_parameters_to_context";
        return false;
    }

    // open codec
    AVDictionary* decoder_options = nullptr;
    defer(av_dict_free(&decoder_options));
    av_dict_set(&decoder_options, "threads", "auto", 0);
    if (avcodec_open2(video_decoder_ctx_, video_decoder_, &decoder_options) < 0) {
        LOG(ERROR) << "avcodec_open2 failed for video\n";
        return false;
    }

    // filter graph
    if (!create_filters()) {
        LOG(ERROR) << "create_filters";
        return false;
    }

    LOG(INFO) << fmt::format("[DECODER] FRAMERATE = {}, CFR = {}, STREAM_TIMEBASE = {}/{}",
                             video_decoder_ctx_->framerate.num, video_decoder_ctx_->framerate.num == video_decoder_ctx_->time_base.den,
                             video_decoder_ctx_->time_base.num, video_decoder_ctx_->time_base.den
    );

    // prepare
	packet_ = av_packet_alloc();
    decoded_video_frame_ = av_frame_alloc();
	filtered_frame_ = av_frame_alloc();

	opened_ = true;
	LOG(INFO) << "[DECODER]: " << name << " is opened";
	return true;
}

bool MediaDecoder::create_filters()
{
	// filters
	filter_graph_ = avfilter_graph_alloc();
	if (!filter_graph_) {
		LOG(ERROR) << "avfilter_graph_alloc";
		return false;
	}

	const AVFilter* buffersrc = avfilter_get_by_name("buffer");
	const AVFilter* buffersink = avfilter_get_by_name("buffersink");
	if (!buffersrc || !buffersink) {
		LOG(ERROR) << "avfilter_get_by_name";
		return false;
	}

	AVStream* video_stream = fmt_ctx_->streams[video_stream_index_];
	string args = fmt::format(
		"video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}",
        video_decoder_ctx_->width, video_decoder_ctx_->height, video_decoder_ctx_->pix_fmt,
		video_stream->time_base.num, video_stream->time_base.den,
		video_stream->sample_aspect_ratio.num, video_stream->sample_aspect_ratio.den
	);

	LOG(INFO) << "[DECODER] " << "buffersrc args : " << args;

	if (avfilter_graph_create_filter(&buffersrc_ctx_, buffersrc, "in", args.c_str(), nullptr, filter_graph_) < 0) {
		LOG(ERROR) << "avfilter_graph_create_filter";
		return false;
	}
	if (avfilter_graph_create_filter(&buffersink_ctx_, buffersink, "out", nullptr, nullptr, filter_graph_) < 0) {
		LOG(ERROR) << "avfilter_graph_create_filter";
		return false;
	}
	enum AVPixelFormat pix_fmts[] = { pix_fmt_, AV_PIX_FMT_NONE };
	if (av_opt_set_int_list(buffersink_ctx_, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) < 0) {
		LOG(ERROR) << "av_opt_set_int_list";
		return false;
	}
	
	if (filters_descr_.length() > 0) {
		AVFilterInOut* inputs = nullptr;
		AVFilterInOut* outputs = nullptr;
		defer(avfilter_inout_free(&inputs));
		defer(avfilter_inout_free(&outputs));
		if (avfilter_graph_parse2(filter_graph_, filters_descr_.c_str(), &inputs, &outputs) < 0) {
			LOG(ERROR) << "avfilter_graph_parse2";
			return false;
		}

		for (auto ptr = inputs; ptr; ptr = ptr->next) {
			if (avfilter_link(buffersrc_ctx_, 0, ptr->filter_ctx, ptr->pad_idx) != 0) {
				LOG(ERROR) << "avfilter_link";
				return false;
			}
		}

		for (auto ptr = outputs; ptr; ptr = ptr->next) {
			if (avfilter_link(ptr->filter_ctx, ptr->pad_idx, buffersink_ctx_, 0) != 0) {
				LOG(ERROR) << "avfilter_link";
				return false;
			}
		}
	}
	else {
		if (avfilter_link(buffersrc_ctx_, 0, buffersink_ctx_, 0) != 0) {
			LOG(ERROR) << "avfilter_link";
			return false;
		}
	}

	if (avfilter_graph_config(filter_graph_, nullptr) < 0) {
		LOG(ERROR) << "avfilter_graph_config(filter_graph_, nullptr)";
		return false;
	}

	LOG(INFO) << "[ENCODER] " << "filter graph @{";
	LOG(INFO) << "\n" << avfilter_graph_dump(filter_graph_, nullptr);
	LOG(INFO) << "[ENCODER] " << "@}";
	return true;
}

void MediaDecoder::start()
{
    if (!opened() || running_) {
        LOG(ERROR) << "[DECODER] already running or not opened";
        return;
    }

    running_ = true;
    video_thread_ = std::thread([this](){ this->video_thread_f(); });
}

void MediaDecoder::video_thread_f()
{
	LOG(INFO) << "[VIDEO THREAD] STARTED@" << std::this_thread::get_id();
    defer(LOG(INFO) << "[VIDEO THREAD] EXITED");

	while (running()) {
        av_packet_unref(packet_);
		int ret = av_read_frame(fmt_ctx_, packet_);
		if (ret < 0) {
            if ((ret == AVERROR_EOF || avio_feof(fmt_ctx_->pb))) {
                LOG(INFO) << "[VIDEO THREAD] PUT NULL PACKET TO FLUSH DECODERS";
                // [flushing] 1. Instead of valid input, send NULL to the avcodec_send_packet() (decoding) or avcodec_send_frame() (encoding) functions. This will enter draining mode.
                // [flushing] 2. Call avcodec_receive_frame() (decoding) or avcodec_receive_packet() (encoding) in a loop until AVERROR_EOF is returned.The functions will not return AVERROR(EAGAIN), unless you forgot to enter draining mode.
                av_packet_unref(packet_);
                packet_->stream_index = video_stream_index_;
            }
            else {
                LOG(ERROR) << "[VIDEO THREAD] read frame failed";
                break;
            }
		}

        if (packet_->stream_index == video_stream_index_) {
            ret = avcodec_send_packet(video_decoder_ctx_, packet_);
            while (ret >= 0) {
                av_frame_unref(decoded_video_frame_);
                ret = avcodec_receive_frame(video_decoder_ctx_, decoded_video_frame_);
                if (ret == AVERROR(EAGAIN)) {
                    break;
                }
                else if (ret == AVERROR_EOF) { // fully flushed, exit
                    // [flushing] 3. Before decoding can be resumed again, the codec has to be reset with avcodec_flush_buffers()
                    avcodec_flush_buffers(video_decoder_ctx_);
                    LOG(INFO) << "[VIDEO THREAD] EOF";
                    return;
                }
                else if (ret < 0) { // error, exit
                    LOG(ERROR) << "[VIDEO THREAD] legitimate decoding errors";
                    return;
                }

				first_pts_ = (first_pts_ == AV_NOPTS_VALUE) ? av_gettime_relative() : first_pts_;
                decoded_video_frame_->pts = (decoded_video_frame_->pts == AV_NOPTS_VALUE) ?
                                            av_rescale_q(av_gettime_relative() - first_pts_, { 1, AV_TIME_BASE }, fmt_ctx_->streams[packet_->stream_index]->time_base) :
                                            decoded_video_frame_->pts - fmt_ctx_->streams[packet_->stream_index]->start_time;

                if (av_buffersrc_add_frame_flags(buffersrc_ctx_, decoded_video_frame_, AV_BUFFERSRC_FLAG_PUSH) < 0) {
                    LOG(ERROR) << "av_buffersrc_add_frame(buffersrc_ctx_, frame_)";
                    break;
                }

                while (true) {
                    av_frame_unref(filtered_frame_);
                    if (av_buffersink_get_frame_flags(buffersink_ctx_, filtered_frame_, AV_BUFFERSINK_FLAG_NO_REQUEST) < 0) {
                        break;
                    }

                    int64_t ts = av_gettime_relative() - first_pts_;

                    int64_t pts_us = av_rescale_q(std::max<int64_t>(0, filtered_frame_->pts), fmt_ctx_->streams[packet_->stream_index]->time_base, { 1, AV_TIME_BASE });
					int64_t duration_us = av_rescale_q(filtered_frame_->pkt_duration, fmt_ctx_->streams[packet_->stream_index]->time_base, { 1, AV_TIME_BASE });
                    int64_t sleep_us = filtered_frame_->pkt_duration > 0 ? duration_us : pts_us - ts;

                    LOG(INFO) << fmt::format("[VIDEO THREAD] pts = {:>6.3f}s, ts = {:>6.3f}s, sleep = {:>4d}ms, frame = {:>4d}, fps = {:>5.2f}",
                                             pts_us / 1000000.0, ts / 1000000.0, sleep_us / 1000,
                                             video_decoder_ctx_->frame_number, video_decoder_ctx_->frame_number / (ts / 1000000.0));

                    av_usleep(sleep_us);

                    video_callback_(filtered_frame_);
                }
            }
        }
	}
}

void MediaDecoder::close()
{
	running_ = false;
    opened_ = false;

    if(video_thread_.joinable()) video_thread_.join();

	first_pts_ = AV_NOPTS_VALUE;

	avfilter_graph_free(&filter_graph_);

    av_packet_free(&packet_);

	av_frame_free(&decoded_video_frame_);
	av_frame_free(&filtered_frame_);

	avcodec_free_context(&video_decoder_ctx_);
	avformat_close_input(&fmt_ctx_);

	LOG(INFO) << "[DECODER] CLOSED";
}
