//
// Created by ruut on 10/23/26.
//

#ifndef SPHLASH_VIDEOWRITER_H
#define SPHLASH_VIDEOWRITER_H

#include <string>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <libavutil/opt.h>
}

class VideoWriter {
public:
    VideoWriter();
    ~VideoWriter();

    bool open(const std::string& filename, int width, int height, int fps);
    bool writeFrame(const uint8_t* rgba);
    bool close();

private:
    bool drainPackets() const;
    static int evenUp(int v);

    AVFormatContext* format = nullptr;
    AVCodecContext* codec = nullptr;
    AVStream* stream = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* packet = nullptr;
    SwsContext* sws = nullptr;

    int width = 0;
    int height = 0;
    int64_t frameIndex = 0;
};

#endif //SPHLASH_VIDEOWRITER_H
