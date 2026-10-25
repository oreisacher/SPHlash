//
// Created by ruut on 10/23/26.
//

#include "utils/VideoWriter.h"

VideoWriter::VideoWriter() {
    av_log_set_level(AV_LOG_QUIET);
}

VideoWriter::~VideoWriter() {
    close();
}

bool VideoWriter::open(const std::string& filename, int width, int height, int fps) {
    this->width = width;
    this->height = height;
    frameIndex = 0;

    if (avformat_alloc_output_context2(&format, nullptr, "mp4", filename.c_str()) < 0 || !format) {
        return false;
    }

    const AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!encoder) return false;

    stream = avformat_new_stream(format, encoder);
    if (!stream) return false;

    codec = avcodec_alloc_context3(encoder);
    if (!codec) return false;

    codec->codec_id = AV_CODEC_ID_H264;
    codec->codec_type = AVMEDIA_TYPE_VIDEO;
    codec->width = evenUp(this->width);
    codec->height = evenUp(this->height);
    codec->time_base = AVRational{1, fps};
    codec->framerate = AVRational{fps, 1};
    codec->pix_fmt = AV_PIX_FMT_YUV420P;
    codec->bit_rate = 8000000;
    codec->gop_size = fps * 2;
    codec->max_b_frames = 0;

    codec->colorspace      = AVCOL_SPC_BT709;
    codec->color_primaries = AVCOL_PRI_BT709;
    codec->color_trc       = AVCOL_TRC_BT709;
    codec->color_range     = AVCOL_RANGE_MPEG;

    av_opt_set(codec->priv_data, "crf", "14", 0);        // 14-18; lower = better
    av_opt_set(codec->priv_data, "preset", "slow", 0);   // "medium"/"fast" for realtime capture
    av_opt_set(codec->priv_data, "profile", "high", 0);

    if (format->oformat->flags & AVFMT_GLOBALHEADER)
        codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(codec, encoder, nullptr) < 0)
        return false;

    if (avcodec_parameters_from_context(
            stream->codecpar, codec) < 0)
        return false;

    stream->time_base = codec->time_base;

    if (!(format->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&format->pb, filename.c_str(),
                      AVIO_FLAG_WRITE) < 0)
            return false;
    }

    if (avformat_write_header(format, nullptr) < 0)
        return false;

    frame = av_frame_alloc();
    packet = av_packet_alloc();

    if (!frame || !packet)
        return false;

    frame->format = codec->pix_fmt;
    frame->width = evenUp(this->width);
    frame->height = evenUp(this->height);

    if (av_frame_get_buffer(frame, 32) < 0)
        return false;

    sws = sws_getContext(
        width, height, AV_PIX_FMT_RGBA,
        evenUp(this->width), evenUp(this->height), AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    return sws != nullptr;
}

bool VideoWriter::writeFrame(const uint8_t* rgba) {
    if (!codec || !frame || !sws || !rgba)
        return false;

    if (av_frame_make_writable(frame) < 0)
        return false;

    const uint8_t* srcData[] = {rgba, nullptr, nullptr, nullptr};
    const int srcStride[] = {width * 4, 0, 0, 0};

    sws_scale(
        sws,
        srcData, srcStride,
        0, height,
        frame->data, frame->linesize);

    frame->pts = frameIndex++;

    if (avcodec_send_frame(codec, frame) < 0)
        return false;

    return drainPackets();
}

bool VideoWriter::drainPackets() const {
    while (true) {
        const int ret = avcodec_receive_packet(codec, packet);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return true;

        if (ret < 0)
            return false;

        av_packet_rescale_ts(
            packet, codec->time_base, stream->time_base);

        packet->stream_index = stream->index;

        const int writeResult =
            av_interleaved_write_frame(format, packet);

        av_packet_unref(packet);

        if (writeResult < 0)
            return false;
    }
}

int VideoWriter::evenUp(int v) {
    return (v + 1) & ~1;
}

bool VideoWriter::close() {
    bool ok = true;

    if (codec && format) {
        // Flush delayed frames from the encoder.
        if (avcodec_send_frame(codec, nullptr) < 0)
            ok = false;
        else if (!drainPackets())
            ok = false;
    }

    if (format && format->pb) {
        if (av_write_trailer(format) < 0)
            ok = false;
    }

    sws_freeContext(sws);
    sws = nullptr;

    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&codec);

    if (format) {
        if (!(format->oformat->flags & AVFMT_NOFILE))
            avio_closep(&format->pb);

        avformat_free_context(format);
        format = nullptr;
    }

    stream = nullptr;
    return ok;
}
