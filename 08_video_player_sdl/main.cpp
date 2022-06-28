#include "logging.h"
#include "videoplayer.h"
#include "SDL.h"

int main(int argc, char *argv[])
{
    Logger::init(argv);

    CHECK(argc >= 2) << "usage : player <input>";
    const char * filename = argv[1];

    CHECK(SDL_Init(SDL_INIT_VIDEO) >= 0);

    VideoPlayer player;
    player.play(filename);

    for(;;) {
        SDL_Event event;
        SDL_WaitEvent(&event);

        switch (event.type) {
            case SDL_USEREVENT + 1:
                player.update();
                break;

            case SDL_QUIT:
                SDL_Quit();
                return 0;

            default: break;
        }
    }
}
