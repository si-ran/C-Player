#pragma once

#include <iostream>
#include <thread>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_RGB_Image.H>
#include <mediaPlayer.h>

struct playerData {
    playUtil* util;
    MediaPlayer* player;
};

void sdlAudioCallback(void* userdata, Uint8* stream, int len) {
    playerData* data = (playerData*)userdata;
    data->player->writeAudioData(stream, len);
    data->player->playVideo(data->util);
}

void startSdlAudio(playerData* data) {
    //--------------------- GET SDL audio READY -------------------
    // audio specs containers
    SDL_AudioDeviceID audioDeviceID;
    SDL_AudioSpec wanted_specs;
    SDL_AudioSpec specs;

    //首次执行，获取samples
    data->player->playVideo(data->util);

    while (true) {
        cout << "getting audio samples." << endl;
        if (data->player->getOutSamples() <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        else {
            cout << "get audio samples:" << data->player->getOutSamples() << endl;
            break;
        }
    }

    // set audio settings from codec info
    wanted_specs.freq = data->player->getOutRate();
    wanted_specs.format = AUDIO_S16SYS;
    wanted_specs.channels = 2;
    wanted_specs.samples = data->player->getOutSamples();
    wanted_specs.callback = sdlAudioCallback;
    wanted_specs.userdata = data;

    // open audio device
    audioDeviceID = SDL_OpenAudioDevice(nullptr, 0, &wanted_specs, &specs, 0);

    // SDL_OpenAudioDevice returns a valid device ID that is > 0 on success or 0 on failure
    if (audioDeviceID == 0) {
        string errMsg = "Failed to open audio device:";
        errMsg += SDL_GetError();
        cout << errMsg << endl;
        throw std::runtime_error(errMsg);
    }

    SDL_PauseAudioDevice(audioDeviceID, 0);
}

int main(int argc, char* argv[]){
    string inputUrl = "F:/Anime/致命女人/Why.Women.Kill.S01E01.中英字幕.WEBrip.720p-人人影视.mp4";
    MediaPlayer* player = new MediaPlayer(inputUrl);

	Fl_Window* window = new Fl_Window(960, 540);
    Fl_Box* box = new Fl_Box(0, 0, 960, 540);
    Fl_Image* old_img = nullptr;
    Fl_Image* new_img = nullptr;
    playUtil* util = new playUtil{ box, old_img, new_img };
    playerData* data = new playerData{util, player};
    std::thread audioThread{ startSdlAudio, data };
    audioThread.detach();
	window->end();
	window->resizable(window);
	Fl::lock();
	window->show(argc, argv);
	return Fl::run();
}