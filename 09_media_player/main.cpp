#include <QApplication>
#include "utils.h"
#include "logging.h"
#include "videoplayer.h"

int main(int argc, char *argv[])
{
    Logger::init(argv);

    QApplication a(argc, argv);
    QApplication::setQuitOnLastWindowClosed(true);

    VideoPlayer player;
    player.show();

    // filepath
    player.play(R"(..\hevc.mkv)");
    // find your camera devices: ffmpeg -hide_banner -f dshow -list_devices true -i dummy
    //player.play("video=HD WebCam", "dshow", "hflip");

    return a.exec();

    return 0;
}
