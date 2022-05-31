#include <QApplication>
#include "logging.h"
#include "videoplayer.h"

int main(int argc, char *argv[])
{
    Logger::init(argv);

    if (argc < 2) {
        LOG(ERROR) << "player <input>";
        return -1;
    }

    QApplication a(argc, argv);
    QApplication::setQuitOnLastWindowClosed(true);

    VideoPlayer player;
    player.show();

    // filepath
    player.play(argv[1]);
    // find your camera devices: ffmpeg -hide_banner -f dshow -list_devices true -i dummy
    //player.play("video=HD WebCam", "dshow", "hflip");

    return a.exec();
}
