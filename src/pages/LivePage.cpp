
#include <iostream>
#include <thread>
#include "pcClient/pages/LiveList.h"
#include "pcClient/pages/LoginPage.h"
#include "pcClient/pages/LivePage.h"
#include "pcClient/ClientUtil.h"
//#include "pcClient/ffmpegUtil.h"
extern "C" {
#include "SDL.h"
};

using namespace ffmpegUtil;

int widthOld = 0;
int fall = 0; //多线程操作元素，只在playVideo中写操作，不知道存不存在问题
int video_delay = 30;

void sdlAudioCallback(void* userdata, Uint8* stream, int len) {
	AudioProcessor* receiver = (AudioProcessor*)userdata;
	receiver->writeAudioData(stream, len);
}

void pktReader(PacketGrabber& pGrabber, AudioProcessor* aProcessor,
	VideoProcessor* vProcessor) {
	const int CHECK_PERIOD = 10;

	cout << "INFO: pkt Reader thread started." << endl;
	int audioIndex = aProcessor->getAudioIndex();
	int videoIndex = vProcessor->getVideoIndex();

	while (!pGrabber.isFileEnd() && !aProcessor->isClosed() && !vProcessor->isClosed()) {
		while (aProcessor->needPacket() || vProcessor->needPacket()) {
			AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));
			int t = pGrabber.grabPacket(packet);
			if (t == -1) {
				cout << "INFO: file finish." << endl;
				aProcessor->pushPkt(nullptr);
				vProcessor->pushPkt(nullptr);
				break;
			}
			else if (t == audioIndex && aProcessor != nullptr) {
				unique_ptr<AVPacket> uPacket(packet);
				aProcessor->pushPkt(std::move(uPacket));
			}
			else if (t == videoIndex && vProcessor != nullptr) {
				unique_ptr<AVPacket> uPacket(packet);
				vProcessor->pushPkt(std::move(uPacket));
			}
			else {
				av_packet_free(&packet);
				cout << "WARN: unknown streamIndex: [" << t << "]" << endl;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_PERIOD));
	}
	cout << "[THREAD] INFO: pkt Reader thread finished." << endl;
}

//void picRefresher(int timeInterval, bool& exitRefresh, bool& faster) {
//	cout << "picRefresher timeInterval[" << timeInterval << "]" << endl;
//	while (!exitRefresh) {
//		if (faster) {
//			std::this_thread::sleep_for(std::chrono::milliseconds(timeInterval / 2));
//		}
//		else {
//			std::this_thread::sleep_for(std::chrono::milliseconds(timeInterval));
//		}
//	}
//	cout << "[THREAD] picRefresher thread finished." << endl;
//}

void playVideo(VideoProcessor* vProcessor, AudioProcessor* audio = nullptr, Fl_Box* box = nullptr, PlayUtil* util = nullptr, Fl_Image* img_old = nullptr, Fl_Image* img_new = nullptr) {
	//--------------------- GET SDL window READY -------------------
	//Sleep(1000);
	auto width = vProcessor->getWidth();
	auto height = vProcessor->getHeight();
	auto frameRate = vProcessor->getFrameRate();
	cout << "frame rate [" << frameRate << "]" << endl;

	bool faster = false;
	//std::thread refreshThread{ picRefresher, (int)(1000 / frameRate), std::ref(exitRefresh),
	//						  std::ref(faster) };

	int failCount = 0;
	while (!vProcessor->isStreamFinished()) {
		fall = 1;
		if (!vProcessor->isClosed()) {
			if (video_delay >= 0) {
				Sleep(video_delay); // 尽量快的延时
			}
			if (vProcessor->isStreamFinished()) {
				continue;  // skip REFRESH event.
			}
			if (audio != nullptr) {
				auto vTs = vProcessor->getPts();
				auto aTs = audio->getPts();
				if (vTs > aTs && vTs - aTs > 30) {
					//video faster
					video_delay++;
					cout << "slower, play video slower" << video_delay << endl;
					continue;
				}
				else if (vTs < aTs && aTs - vTs > 30) {
					video_delay--;
					cout << "slower, play video faster" << video_delay << endl;
					// video slower
				}
				else {
					//faster = false;
				}
			}
			if (vProcessor != nullptr) {
				auto pic = vProcessor->getFrame();
				if (pic != nullptr) {
					if (widthOld != box->w()) {
						widthOld = box->w();
						util->window->redraw();
					}
					Fl_RGB_Image* image2 = new Fl_RGB_Image(pic->data[0], width, height);
					img_old = img_new;
					delete(img_old);
					img_new = image2->copy(box->w() - 5, box->h() - 5);
					Fl::lock();
					box->image(img_new);
					box->redraw();
					Fl::unlock();
					Fl::awake();
					delete (image2);
					//delete (image);

					if (!vProcessor->refreshFrame()) {
						cout << "WARN: vProcessor.refreshFrame false" << endl;
					}
				}
				else {
					failCount++;
					cout << "WARN: getFrame fail. failCount = " << failCount << endl;
				}
			}
		}
		else {
			cout << "close video." << endl;
			// close window.
			break;
		}
	}
	fall = 0;
	//refreshThread.join();
	cout << "[THREAD] Sdl video thread finish: failCount = " << failCount << endl;
}

void startSdlAudio(AudioProcessor& aProcessor) {
	//--------------------- GET SDL audio READY -------------------
	//Sleep(1500);
	// audio specs containers
	SDL_AudioDeviceID audioDeviceID;
	SDL_AudioSpec wanted_specs;
	SDL_AudioSpec specs;

	cout << "aProcessor.getSampleFormat() = " << aProcessor.getSampleFormat() << endl;
	cout << "aProcessor.getSampleRate() = " << aProcessor.getOutSampleRate() << endl;
	cout << "aProcessor.getChannels() = " << aProcessor.getOutChannels() << endl;

	int samples = -1;
	while (true) {
		cout << "getting audio samples." << endl;
		samples = aProcessor.getSamples();
		if (samples <= 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		else {
			cout << "get audio samples:" << samples << endl;
			break;
		}
	}

	// set audio settings from codec info
	wanted_specs.freq = aProcessor.getOutSampleRate();
	wanted_specs.format = AUDIO_S16SYS;
	wanted_specs.channels = aProcessor.getOutChannels();
	wanted_specs.samples = samples;
	wanted_specs.callback = sdlAudioCallback;
	wanted_specs.userdata = &aProcessor;

	// open audio device
	audioDeviceID = SDL_OpenAudioDevice(nullptr, 0, &wanted_specs, &specs, 0);

	// SDL_OpenAudioDevice returns a valid device ID that is > 0 on success or 0 on failure
	if (audioDeviceID == 0) {
		string errMsg = "Failed to open audio device:";
		errMsg += SDL_GetError();
		cout << errMsg << endl;
		throw std::runtime_error(errMsg);
	}
	cout << "audioDeviceID" << " : " << audioDeviceID << endl;

	cout << "wanted_specs.freq:" << wanted_specs.freq << endl;
	// cout << "wanted_specs.format:" << wanted_specs.format << endl;
	std::printf("wanted_specs.format: Ox%X\n", wanted_specs.format);
	cout << "wanted_specs.channels:" << (int)wanted_specs.channels << endl;
	cout << "wanted_specs.samples:" << (int)wanted_specs.samples << endl;

	cout << "------------------------------------------------" << endl;

	cout << "specs.freq:" << specs.freq << endl;
	// cout << "specs.format:" << specs.format << endl;
	std::printf("specs.format: Ox%X\n", specs.format);
	cout << "specs.channels:" << (int)specs.channels << endl;
	cout << "specs.silence:" << (int)specs.silence << endl;
	cout << "specs.samples:" << (int)specs.samples << endl;


	SDL_PauseAudioDevice(audioDeviceID, 0);
	//cout << SDL_GetError() << " : " << audioDeviceID << endl;
	//cout << "[THREAD] audio start thread finish." << endl;
}

LivePage::LivePage(Fl_Window* window, LiveInfo liveInfo) : Fl_Group(0, 0, window->w(), window->h()) {
	window->label(ClientUtil::fl_str("直播房间"));

	info = new Fl_Group(30, 20, 230, 500);
	string roomIdText = "";
	roomIdText = roomIdText + "房间号: " + ClientUtil::toString<long>(liveInfo.roomId);
	roomId = new Fl_Box(40, 40, 0, 0, ClientUtil::fl_cstr((roomIdText.c_str())));
	roomId->align(FL_ALIGN_RIGHT);
	roomId->labelsize(18);
	roomUser = new Fl_Box(40, 70, 0, 0, ClientUtil::fl_cstr(("主播: " + liveInfo.name).c_str()));
	roomUser->align(FL_ALIGN_RIGHT);
	roomUser->labelsize(18);
	btn1 = new Fl_Button(100, 480, 90, 30, ClientUtil::fl_str("退出"));
	btn1->box(_FL_ROUND_UP_BOX);
	btn1->clear_visible_focus();
	info->end();

	video = new Fl_Box(290, 20, 640, 360);
	video->box(FL_BORDER_FRAME);
	video->align(FL_ALIGN_IMAGE_BACKDROP);

	box_cmt_image = new Fl_Text_Display(290, 420, 640, 100);
	box_cmt_image->align(FL_ALIGN_TOP_LEFT);
	box_cmt_image->box(FL_BORDER_BOX);
	text_buffer = new Fl_Text_Buffer();
	text_buffer->text(ClientUtil::fl_str("还没有留言"));
	box_cmt_image->buffer(text_buffer);

	//播放视频的初始化
	try {
		packetGrabber = new ffmpegUtil::PacketGrabber{ liveInfo.url };
		auto formatCtx = packetGrabber->getFormatCtx();
		av_dump_format(formatCtx, 0, "", 0);
		videoProcessor = new VideoProcessor(formatCtx);
		videoProcessor->start();
		audioProcessor = new AudioProcessor(formatCtx);
		audioProcessor->start();
		SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE", "1", 1);
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
			string errMsg = "Could not initialize SDL -";
			errMsg += SDL_GetError();
			cout << errMsg << endl;
			throw std::runtime_error(errMsg);
		}

		myUtil = new PlayUtil{ window, videoProcessor, audioProcessor };
		std::thread readerThread{ pktReader, std::ref(*packetGrabber), audioProcessor, videoProcessor };
		std::thread startAudioThread(startSdlAudio, std::ref(*audioProcessor));
		std::thread videoThread{ playVideo, videoProcessor, audioProcessor, video, myUtil, image_old, image_new };
		readerThread.detach();
		startAudioThread.detach();
		videoThread.detach();
		btn1->callback((Fl_Callback*)switchPage, myUtil);
	}
	catch (std::exception ex) {
		fl_message(ClientUtil::fl_str("打开rtmp流失败，因为打开时间过长"));
		cout << "got exception:" << ex.what() << endl;
		btn1->callback((Fl_Callback*)switchPageNull, window);
	}
	this->take_focus();
	this->end();
}

static void closeCallback(void* utils) {
	if (!fall) {
		PlayUtil* util = (PlayUtil*)utils;
		util->window->clear();
		util->window->begin();
		Fl_Group* group = new LiveList(util->window);
		util->window->end();
		util->window->redraw();
	}
	else {
		Fl::repeat_timeout(0.1, closeCallback, utils);
	}
}

void LivePage::switchPage(Fl_Button* btn, PlayUtil* util) {
	SDL_Quit();
	//SDL_CloseAudioDevice(2);
	cout << util->videoProcessor->close() << endl;
	cout << util->audioProcessor->close() << endl;
	//等待元素绘制完成
	Fl::add_timeout(0.1, closeCallback, util);
}

void LivePage::switchPageNull(Fl_Button* btn, Fl_Window* window) {
	window->clear();
	window->begin();
	Fl_Group* group = new LiveList(window);
	window->end();
	window->redraw();
}

int LivePage::handle(int e) {
	int ret = Fl_Group::handle(e);
	switch (e) {
	case FL_PUSH:
		cout << "push" << " event and returns:" << ret << endl;
		break;
	case FL_KEYDOWN:
		if (Fl::event_key() == 115) {
			if (myUtil->isFullscreen) {
				myUtil->window->fullscreen_off(0, 0, 960, 540);
				video->resize(290, 20, 640, 360);
				myUtil->window->redraw();
				myUtil->isFullscreen = false;
			}
			else {
				myUtil->window->fullscreen();
				video->resize(0, 0, myUtil->window->w(), myUtil->window->h());
				myUtil->isFullscreen = true;
			}
		}
		else if (Fl::event_key() == 65289) {
			if (myUtil->isInfoscreen) {
				video->label("");
				myUtil->isInfoscreen = false;
			}
			else {
				time_t times = time(NULL);
				struct tm* stime = localtime(&times);
				char tmp[32] = { NULL };
				sprintf(tmp, "%04d-%02d-%02d %02d:%02d:%02d", 1900 + stime->tm_year, 1 + stime->tm_mon,
					stime->tm_mday, stime->tm_hour,
					stime->tm_min, stime->tm_sec);
				cout << "本地时间：" << tmp << endl;

				ffmpegUtil::FFInfo info = packetGrabber->getInfo();
				string str = "";
				str = str + "视频分辨率：" + ClientUtil::toString<int>(info.width) + "*" + ClientUtil::toString<int>(info.height) + "\n";
				str = str + "视频总帧数：" + ClientUtil::toString<int>(info.frame_number) + "\n";
				str = str + "视频帧率：" + ClientUtil::toString<int>(info.frame_rate) + "\n";
				str = str + "音频采样率：" + ClientUtil::toString<int>(info.audio_samplerate) + "\n";
				str = str + tmp;
				video->label(ClientUtil::fl_cstr(str.c_str()));
				myUtil->isInfoscreen = true;
			}
		}
		break;
	default: 
		break;
	}
	return ret;
}