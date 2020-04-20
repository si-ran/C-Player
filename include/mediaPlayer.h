#pragma once

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_RGB_Image.H>
#include <ffmpegUtil.h>

struct playUtil {
    Fl_Box* box;
    Fl_Image* old_img;
    Fl_Image* new_img;
};

class MediaPlayer {
private:
    int outSamples = -1;
    int outDataSize = -1;

    SwsContext* sws_ctx = nullptr;
    SwrContext* swr = nullptr;
    AVPacket* Pkt = nullptr;
    AVFrame* getPic = nullptr;
    AVFrame* outPic = nullptr;
    AVFrame* getAudio = nullptr;
    uint8_t* outBuffer = nullptr;
    AVCodecContext* videoCodecCtx = nullptr;
    AVCodecContext* audioCodecCtx = nullptr;
    AVFormatContext* formatCtx = nullptr;

    int videoIndex = -1;
    int audioIndex = -1;
public:
	MediaPlayer(string url) {
        formatCtx = avformat_alloc_context();
        if (avformat_open_input(&formatCtx, url.c_str(), NULL, NULL) != 0) {
            string errorMsg = "Can not open input file:";
            errorMsg += url;
            cout << errorMsg << endl;
            throw std::runtime_error(errorMsg);
        }
        if (avformat_find_stream_info(formatCtx, NULL) < 0) {
            string errorMsg = "Can not find stream information in input file:";
            errorMsg += url;
            cout << errorMsg << endl;
            throw std::runtime_error(errorMsg);
        }
        videoIndex = av_find_best_stream(formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, NULL);
        audioIndex = av_find_best_stream(formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, NULL);
        cout << "video stream index = : [" << videoIndex << "]" << endl;
        cout << "audio stream index = : [" << audioIndex << "]" << endl;

        //获取AVCodecContext-video
        AVCodec* videoCodec = avcodec_find_decoder(formatCtx->streams[videoIndex]->codecpar->codec_id);
        if (videoCodec == nullptr) {
            string errorMsg = "video Could not find codec: ";
            cout << errorMsg << endl;
            throw std::runtime_error(errorMsg);
        }
        videoCodecCtx = avcodec_alloc_context3(videoCodec);
        if (avcodec_parameters_to_context(videoCodecCtx, formatCtx->streams[videoIndex]->codecpar) != 0) {
            string errorMsg = "video Could not copy codec context: ";
            cout << errorMsg << endl;
            throw std::runtime_error(errorMsg);
        }
        if (avcodec_open2(videoCodecCtx, videoCodec, nullptr) < 0) {
            string errorMsg = "video Could not open codec: ";
            cout << errorMsg << endl;
            throw std::runtime_error(errorMsg);
        }

        //获取AVCodecContext-audio
        AVCodec* audioCodec = avcodec_find_decoder(formatCtx->streams[audioIndex]->codecpar->codec_id);
        if (audioCodec == nullptr) {
            string errorMsg = "audio Could not find codec: ";
            cout << errorMsg << endl;
            throw std::runtime_error(errorMsg);
        }
        audioCodecCtx = avcodec_alloc_context3(audioCodec);
        if (avcodec_parameters_to_context(audioCodecCtx, formatCtx->streams[audioIndex]->codecpar) != 0) {
            string errorMsg = "audio Could not copy codec context: ";
            cout << errorMsg << endl;
            throw std::runtime_error(errorMsg);
        }
        if (avcodec_open2(audioCodecCtx, audioCodec, nullptr) < 0) {
            string errorMsg = "audio Could not open codec: ";
            cout << errorMsg << endl;
            throw std::runtime_error(errorMsg);
        }

        //初始化sws
        int w = videoCodecCtx->width;
        int h = videoCodecCtx->height;
        sws_ctx = sws_getContext(w, h, videoCodecCtx->pix_fmt, w, h, AV_PIX_FMT_RGB24, SWS_BILINEAR,
            NULL, NULL, NULL);
        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, w, h, 32);
        outPic = av_frame_alloc();
        getPic = av_frame_alloc();
        uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
        av_image_fill_arrays(outPic->data, outPic->linesize, buffer, AV_PIX_FMT_RGB24, w, h, 32);

        //初始化swr
        int64_t inLayout = audioCodecCtx->channel_layout;
        int inSampleRate = audioCodecCtx->sample_rate;
        int inChannels = audioCodecCtx->channels;
        AVSampleFormat inFormat = audioCodecCtx->sample_fmt;

        int64_t layout = AV_CH_LAYOUT_STEREO;
        int sampleRate = inSampleRate;
        int channels = 2;
        AVSampleFormat format = AV_SAMPLE_FMT_S16;

        swr = swr_alloc_set_opts(nullptr, layout, format, sampleRate, inLayout, inFormat, inSampleRate, 0, nullptr);
        if (swr_init(swr)) {
            throw std::runtime_error("swr_init error.");
        }
        getAudio = av_frame_alloc();

        //初始化sdl
        SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE", "1", 1);
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
            string errMsg = "Could not initialize SDL -";
            errMsg += SDL_GetError();
            cout << errMsg << endl;
            throw std::runtime_error(errMsg);
        }
	}

    void playVideo(playUtil* util) {
        while (true) {
            Pkt = (AVPacket*)av_malloc(sizeof(AVPacket));
            if (av_read_frame(formatCtx, Pkt) < 0) {
                string errorMsg = "read packet error: ";
                cout << errorMsg << endl;
                throw std::runtime_error(errorMsg);
            }
            //audio
            if (Pkt->stream_index == audioIndex) {
                int ret = -1;
                ret = avcodec_send_packet(audioCodecCtx, Pkt);
                if (ret == 0) {
                    av_packet_free(&Pkt);
                    Pkt = nullptr;
                }
                else if (ret == AVERROR(EAGAIN)) {
                    // buff full, can not decode any more, nothing need to do.
                }
                else if (ret == AVERROR_EOF) {
                    // no new packets can be sent to it, it is safe.
                }
                else {
                    string errorMsg = "avcodec_send_packet error: ";
                    errorMsg += ret;
                    cout << errorMsg << endl;
                    throw std::runtime_error(errorMsg);
                }
                ret = avcodec_receive_frame(audioCodecCtx, getAudio);
                if (ret == 0) {
                    int guessOutSize = av_rescale_rnd(getAudio->nb_samples, audioCodecCtx->sample_rate, audioCodecCtx->sample_rate, AV_ROUND_UP) * 4.8;
                    if (outBuffer == nullptr) {
                        outBuffer = (uint8_t*)av_malloc(sizeof(uint8_t) * guessOutSize);
                    }
                    else {
                        memset(outBuffer, 0, guessOutSize);
                    }

                    outSamples = swr_convert(swr, &outBuffer, guessOutSize,
                        (const uint8_t**)&getAudio->data[0], getAudio->nb_samples);
                    if (outSamples <= 0) {
                        throw std::runtime_error("error: outSamples=" + outSamples);
                    }

                    outDataSize =
                        av_samples_get_buffer_size(NULL, 2, outSamples, AV_SAMPLE_FMT_S16, 1);
                    if (outDataSize <= 0) {
                        throw std::runtime_error("error: outDataSize=" + outDataSize);
                    }
                    break;
                }
                else if (ret == AVERROR_EOF) {
                    cout << "MediaProcessor no more output frames. index=" << endl;
                    //streamFinished = true;
                }
                else if (ret == AVERROR(EAGAIN)) {
                    // need more packet.
                }
                else {
                    string errorMsg = "avcodec_receive_frame error: ";
                    errorMsg += ret;
                    cout << errorMsg << endl;
                    throw std::runtime_error(errorMsg);
                }
            }
            //video
            else if (Pkt->stream_index == videoIndex) {
                int ret = -1;
                ret = avcodec_send_packet(videoCodecCtx, Pkt);
                if (ret == 0) {
                    av_packet_free(&Pkt);
                    Pkt = nullptr;
                }
                else if (ret == AVERROR(EAGAIN)) {
                    // buff full, can not decode any more, nothing need to do.
                }
                else if (ret == AVERROR_EOF) {
                    // no new packets can be sent to it, it is safe.
                }
                else {
                    string errorMsg = "avcodec_send_packet error: ";
                    errorMsg += ret;
                    cout << errorMsg << endl;
                    throw std::runtime_error(errorMsg);
                }
                ret = avcodec_receive_frame(videoCodecCtx, getPic);
                if (ret == 0) {
                    auto t = getPic->pts * av_q2d(formatCtx->streams[videoIndex]->time_base) * 1000;
                    sws_scale(sws_ctx, (uint8_t const* const*)getPic->data, getPic->linesize, 0,
                        videoCodecCtx->height, outPic->data, outPic->linesize);
                    Fl_RGB_Image* image = new Fl_RGB_Image(outPic->data[0], videoCodecCtx->width, videoCodecCtx->height);
                    util->old_img = util->new_img;
                    util->new_img = image->copy(util->box->w() - 5, util->box->h() - 5);
                    Fl::lock();
                    util->box->image(util->new_img);
                    util->box->redraw();
                    Fl::unlock();
                    Fl::awake();
                    delete(util->old_img);
                    delete(image);
                }
                else if (ret == AVERROR_EOF) {
                    cout << "MediaProcessor no more output frames. index=" << endl;
                    //streamFinished = true;
                }
                else if (ret == AVERROR(EAGAIN)) {
                    // need more packet.
                }
                else {
                    string errorMsg = "avcodec_receive_frame error: ";
                    errorMsg += ret;
                    cout << errorMsg << endl;
                    throw std::runtime_error(errorMsg);
                }
            }
        }
    }

    void writeAudioData(Uint8* stream, int len) {
        static uint8_t* silenceBuff = nullptr;
        if (silenceBuff == nullptr) {
            silenceBuff = (uint8_t*)av_malloc(sizeof(uint8_t) * len);
            std::memset(silenceBuff, 0, len);
        }

        if (outDataSize != len) {
            cout << "WARNING: outDataSize[" << outDataSize << "] != len[" << len << "]" << endl;
        }
        std::memcpy(stream, outBuffer, outDataSize);
    }

    int getOutSamples() { return outSamples; }
    int getOutRate() { return audioCodecCtx->sample_rate; }
};