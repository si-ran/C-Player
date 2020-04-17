#include <iostream>
#include <thread>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_RGB_Image.H>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
};

using std::string;
using std::cout;
using std::endl;


Fl_Image* old_img = nullptr;
Fl_Image* new_img = nullptr;


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

void playVideo(Fl_Box* box, Fl_Image* old_img, Fl_Image* new_img) {
    while (true) {
        Sleep(5);
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
                int guessOutSize = av_rescale_rnd(getAudio->nb_samples, audioCodecCtx->sample_rate, audioCodecCtx->sample_rate, AV_ROUND_UP) * 4;
                if (outBuffer == nullptr) {
                    outBuffer = (uint8_t*)av_malloc(sizeof(uint8_t) * guessOutSize);
                }
                else {
                    memset(outBuffer, 0, guessOutSize);
                }
                swr_convert(swr, &outBuffer, guessOutSize,
                    (const uint8_t**)&getAudio->data[0], getAudio->nb_samples);
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
                sws_scale(sws_ctx, (uint8_t const* const*)getPic->data, getPic->linesize, 0,
                    videoCodecCtx->height, outPic->data, outPic->linesize);
                Fl_RGB_Image* image = new Fl_RGB_Image(outPic->data[0], videoCodecCtx->width, videoCodecCtx->height);
                old_img = new_img;
                new_img = image->copy(box->w() - 5, box->h() - 5);
                Fl::lock();
                box->image(new_img);
                box->redraw();
                Fl::unlock();
                Fl::awake();
                delete(old_img);
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

int main(int argc, char** argv){

    string inputUrl = "F:/Anime/kiss sister/test.mp4";
    formatCtx = avformat_alloc_context();
    if (avformat_open_input(&formatCtx, inputUrl.c_str(), NULL, NULL) != 0) {
        string errorMsg = "Can not open input file:";
        errorMsg += inputUrl;
        cout << errorMsg << endl;
        throw std::runtime_error(errorMsg);
    }
    if (avformat_find_stream_info(formatCtx, NULL) < 0) {
        string errorMsg = "Can not find stream information in input file:";
        errorMsg += inputUrl;
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

    //for (int i = 0; i < formatCtx->nb_streams; i++) {
    //    AVStream* in_stream = formatCtx->streams[i];
    //    if (formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && videoIndex == -1) {
    //        videoIndex = i;
    //        cout << "video stream index = : [" << i << "]" << endl;
    //        cout << "video width " << in_stream->codec->width << endl;
    //        cout << "video height " << in_stream->codec->height << endl;
    //        //v_width = in_stream->codec->width;
    //        //v_height = in_stream->codec->height;
    //        if (in_stream->avg_frame_rate.den != 0 && in_stream->avg_frame_rate.num != 0){
    //            cout << "frame rate " << in_stream->avg_frame_rate.num / in_stream->avg_frame_rate.den <<endl;//每秒多少帧 
    //        }
    //        //video_frame_count = in_stream->nb_frames; //视频总帧数
    //    }
    //    if (formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audioIndex == -1) {
    //        audioIndex = i;
    //        cout << "audio stream index = : [" << i << "]" << endl;
    //    }
    //}
	Fl_Window* window = new Fl_Window(960, 540);
    Fl_Box* box = new Fl_Box(0, 0, 960, 540);
    std::thread videoThread{ playVideo, box, old_img, new_img };
    videoThread.detach();
	window->end();
	window->resizable(window);
	Fl::lock();
	window->show(argc, argv);
	return Fl::run();
}