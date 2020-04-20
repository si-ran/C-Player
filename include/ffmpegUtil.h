#pragma once
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
};
extern "C" {
#include "SDL.h"
}

#undef main 

using std::string;
using std::cout;
using std::endl;