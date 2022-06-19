#ifndef _05_FILTER_GRAPH_H
#define _05_FILTER_GRAPH_H

#include <vector>
#include <string>
#include <thread>
#include <atomic>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}

#include "defer.h"
#include "logging.h"
#include "ringvector.h"
#include "fmt/format.h"

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
        CHECK_NOTNULL(buffersrc);

        AVFilterContext * filter_ctx = nullptr;
        CHECK(avfilter_graph_create_filter(&filter_ctx, buffersrc, "in", args.c_str(), nullptr, filter_graph_) >= 0);

        buffersrc_ctxs_.push_back(filter_ctx);
        return 0;
    }

    int create(const std::string& descr)
    {
        LOG(INFO) << "create filter for: " << descr;

        const AVFilter *buffersink = avfilter_get_by_name("buffersink");
        CHECK_NOTNULL(buffersink);

        enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
        CHECK(avfilter_graph_create_filter(&buffersink_ctx_, buffersink, "sink", nullptr, nullptr, filter_graph_) >= 0);
        CHECK(av_opt_set_int_list(buffersink_ctx_, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) >= 0);

        AVFilterInOut* inputs = nullptr;
        AVFilterInOut* outputs = nullptr;
        CHECK(avfilter_graph_parse2(filter_graph_, descr.c_str(), &inputs, &outputs) >= 0);

        int i = 0;
        for (auto ptr = inputs; ptr; ptr = ptr->next, i++) {
            CHECK(avfilter_link(buffersrc_ctxs_[i], 0, ptr->filter_ctx, ptr->pad_idx) >= 0);
        }

        for (auto ptr = outputs; ptr; ptr = ptr->next) {
            CHECK(avfilter_link(ptr->filter_ctx, ptr->pad_idx, buffersink_ctx_, 0) >= 0);
        }

        CHECK(avfilter_graph_config(filter_graph_, nullptr) >= 0);

        char * graph = avfilter_graph_dump(filter_graph_, nullptr);
        CHECK_NOTNULL(graph);

        LOG(INFO) << "graph >>>> \n%s\n" << graph;

        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);

        return 0;
    }

    AVRational time_base() const { return av_buffersink_get_time_base(buffersink_ctx_); }
    AVRational sample_aspect_ratio() const { return av_buffersink_get_sample_aspect_ratio(buffersink_ctx_); }
    int height() const { return av_buffersink_get_h(buffersink_ctx_); }
    int width() const { return av_buffersink_get_w(buffersink_ctx_); }
    AVRational framerate() const { return av_buffersink_get_frame_rate(buffersink_ctx_); }
    AVPixelFormat format() const { return (AVPixelFormat)av_buffersink_get_format(buffersink_ctx_); }

    //private:
    std::atomic<bool> running_{false};

    AVFilterGraph * filter_graph_{nullptr};
    std::vector<AVFilterContext*> buffersrc_ctxs_{};
    AVFilterContext* buffersink_ctx_{ nullptr };
};

#endif //!_05_FILTER_GRAPH_H