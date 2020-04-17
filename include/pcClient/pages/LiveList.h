#pragma once

#include<FL/Fl.H>
#include<FL/Fl_Window.H>
#include<FL/Fl_Box.H>
#include<FL/Fl_Button.H>
#include<vector>
#include <nlohmann/json.hpp>

using nlohmann::json;
using std::string;
using std::vector;


struct RoomInfo {
	long roomId;
	string userName;
	string rtmpUrl;
	//string mpdUrl;
	int roomSate;
};
struct GetRoomInfoRsp {
	vector<RoomInfo> roomList;
	int errCode;
	string msg;
};
struct EnterInfo {
	long roomId;
	string userName;
	string rtmpUrl;
	//string mpdUrl;
	int roomState;
	Fl_Window* window;
};

class LiveList : public Fl_Group {
public:
	LiveList(Fl_Window* window);
private:
	static void switchPage1(Fl_Button* btn, Fl_Window*);
	static void switchPage2(Fl_Button* btn, EnterInfo*);
	static void getRoomListFunc(string& bodyString);

	Fl_Button* btn1;
	Fl_Button* btn2;
	Fl_Box* box1;

	Fl_Box* roomIdBox;
	Fl_Box* controlBox;


}; 


