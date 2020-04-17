#pragma once

#include<FL/Fl.H>
#include<FL/Fl_Window.H>
#include<FL/Fl_Box.H>
#include<FL/Fl_Button.H>
#include<FL/FL_Text_Display.H>
#include<FL/Fl_PNG_Image.H>
#include<FL/Fl_Shared_Image.H>
#include<FL/fl_draw.H>
#include <FL/fl_ask.H>
#include"pcClient/MediaProcessor.hpp"
#include"pcClient/ffmpegUtil.h"

struct PlayUtil {
	Fl_Window* window;
	VideoProcessor* videoProcessor;
	AudioProcessor* audioProcessor;
	std::thread thread;
	boolean isFullscreen = false;
	boolean isInfoscreen = false;
	string time = "";
};

struct LiveInfo {
	long roomId;
	string name;
	string url;
};

class LivePage : public Fl_Group {
public:
	LivePage(Fl_Window* window, LiveInfo url);
	void playRGB(void*);
	void playAudio(void*);
	int handle(int e);
private:
	static void switchPage(Fl_Button* btn, PlayUtil*);
	static void switchPageNull(Fl_Button* btn, Fl_Window*);
	Fl_Button* btn1;
	PlayUtil* myUtil;
	Fl_Box* video;
	Fl_Group* info;
	Fl_Box* roomId;
	Fl_Box* roomUser;
	Fl_Image* image_old = nullptr;
	Fl_Image* image_new = nullptr;
	Fl_Text_Display* box_cmt_image;
	Fl_Text_Buffer* text_buffer;

	//ffmpegUtil::PacketGrabber packetGrabber{ "F:/Anime/kiss sister/test.mp4" };
	ffmpegUtil::PacketGrabber* packetGrabber;
	VideoProcessor* videoProcessor;
	AudioProcessor* audioProcessor;
	//TestMedia media;
};

