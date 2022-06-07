#include "audio-io.h"

int main(int argc, char* argv[])
{
    enum_audio_endpoints();
    default_audio_endpoint();

    MyAudioSink sink;
    record_audio_stream(&sink);

    return 0;
}