#ifndef FFMPEG_EXAMPLES_LOGGING_H
#define FFMPEG_EXAMPLES_LOGGING_H

#include <glog/logging.h>

class Logger 
{
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static Logger& init(char* argv0) 
    {
        static Logger logger(argv0);
        return logger;
    }

    ~Logger() 
    {
        google::ShutdownGoogleLogging();
    }

private:
    explicit Logger(char* argv0) 
    {
        google::InitGoogleLogging(
            argv0,
            [](std::ostream& _stream, const google::LogMessageInfo& _info, void*)
            {
                std::string _file_line = _info.filename + std::string(":") + std::to_string(_info.line_number);
                _file_line = _file_line.substr(std::max<int64_t>(0, _file_line.size() - 24), 24);

                _stream
                    << std::setw(4) << 1900 + _info.time.year() << '-'
                    << std::setw(2) << 1 + _info.time.month() << '-'
                    << std::setw(2) << _info.time.day()
                    << ' '
                    << std::setw(2) << _info.time.hour() << ':'
                    << std::setw(2) << _info.time.min() << ':'
                    << std::setw(2) << _info.time.sec() << "."
                    << std::setw(3) << _info.time.usec() / 1000
                    << std::setfill(' ')
                    << ' '
                    << std::setw(7) << _info.severity
                    << ' '
                    << std::setw(5) << _info.thread_id
                    << " -- ["
                    << std::setw(24) << _file_line << "]:";
            }
        );


        FLAGS_logbufsecs = 0;
        FLAGS_stderrthreshold = google::GLOG_INFO;
        FLAGS_colorlogtostderr = true;
        google::InstallFailureSignalHandler();
        google::InstallFailureWriter([](const char* data, size_t size) {
            LOG(ERROR) << std::string(data, size);
        });
    }
};
#endif // !FFMPEG_EXAMPLES_LOGGING_H
