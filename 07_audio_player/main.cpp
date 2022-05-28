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
    player.play(R"(..\..\hevc.mkv)");

    return a.exec();
}
