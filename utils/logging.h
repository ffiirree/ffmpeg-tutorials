#ifndef PLAYER_LOGGING_H
#define PLAYER_LOGGING_H

#include <glog/logging.h>

class Logger {
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static Logger& init(char** argv) {
        static Logger logger(argv);
        return logger;
    }

    ~Logger() {
        google::ShutdownGoogleLogging();
    }

private:
    Logger(char** argv) {
        google::InitGoogleLogging(argv[0]);
        FLAGS_logbufsecs = 0;
        FLAGS_stderrthreshold = google::GLOG_INFO;
        FLAGS_colorlogtostderr = true;
    }
};
#endif // !PLAYER_LOGGING_H
