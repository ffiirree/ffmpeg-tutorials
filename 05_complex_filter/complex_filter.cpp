#include "encoder.h"
#include "decoder.h"
#include "filter_graph.h"

int main(int argc, char* argv[])
{
    if (argc < 4) {
        LOG(ERROR) << "complex_filter -i <input-watermark> -i <input-video> <output>";
        return -1;
    }

    std::vector<std::string> input_files;
    std::string output_file;

    std::vector<std::shared_ptr<Decoder>> decoders;
    std::vector<std::thread> threads;
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
        CHECK(decoder->open(input) >= 0);
        decoders.push_back(decoder);

        filter.create_buffersrc(decoder->filter_args());
    }

    const std::string filter_complex = "[0:v] scale=64:-1:flags=lanczos [s];[1:v][s]overlay=10:10";
    filter.create(filter_complex);
    LOG(INFO) << fmt::format(R"( -- same as : ffmpeg -i {} -i {} -filter_complex "{}" {})", input_files[0], input_files[1], filter_complex, output_file);

    encoder.open(output_file, filter.width(), filter.height(), filter.format(), filter.sample_aspect_ratio(), filter.framerate(), filter.time_base());

    for (auto & decoder : decoders) {
        threads.emplace_back(std::thread([&](){ decoder->running_ = true; decoder->decode_thread(); }));
    }

    LOG(INFO) << "[FILTER THREAD] START @ " << std::this_thread::get_id();
    AVFrame * frame = av_frame_alloc();
    AVFrame * filtered_frame = av_frame_alloc();

    filter.running_ = true;
    while(filter.running_) {
        for(int i = 0; i < decoders.size(); i++) {
            if (decoders[i]->video_frame_buffer_.empty()) {
                if (!decoders[i]->eof())
                    av_usleep(20000);
                continue;
            }

            decoders[i]->video_frame_buffer_.pop([&](AVFrame * popped) {
                av_frame_unref(frame);
                av_frame_move_ref(frame, popped);
            });

            int ret = av_buffersrc_add_frame_flags(filter.buffersrc_ctxs_[i], (!frame->width && !frame->height) ? nullptr : frame, AV_BUFFERSRC_FLAG_PUSH);
            while(ret >= 0) {
                av_frame_unref(filtered_frame);
                ret = av_buffersink_get_frame(filter.buffersink_ctx_, filtered_frame);
                if (ret == AVERROR(EAGAIN)) {
                    break;
                }
                else if (ret == AVERROR_EOF) {
                    LOG(INFO) << "[FILTER THREAD] EOF";
                    av_frame_unref(filtered_frame);
                    filter.running_ = false;
                }
                else if (ret < 0) {
                    LOG(ERROR) << "av_buffersink_get_frame_flags()";
                    filter.running_ = false;
                    break;
                }

                encoder.encode_frame(filtered_frame);
            }
        }
    }

    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    LOG(INFO) << "EXITED";

    av_frame_free(&filtered_frame);
    av_frame_free(&frame);

    return 0;
}