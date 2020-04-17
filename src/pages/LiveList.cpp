#include <iostream>
#include "pcClient/HttpUtil.h"
#include "pcClient/pages/LiveList.h"
#include "pcClient/pages/LoginPage.h"
#include "pcClient/pages/LivePage.h"
#include<FL/Fl.H>
#include<FL/Fl_Window.H>
#include<FL/Fl_Box.H>
#include<FL/Fl_Button.H>
#include <vector>
#include <nlohmann/json.hpp>
#include "pcClient/ClientUtil.h"

using nlohmann::json;
using std::string;
using std::vector;

void to_json(json& j, const RoomInfo& r) {
	j = json{ {"roomId", r.roomId},{"userName", r.userName}, {"rtmpUrl", r.rtmpUrl}/*, {"mpdUrl", r.mpdUrl}*/, {"roomSate", r.roomSate} };
}
void from_json(const json& j, RoomInfo& r) {
	r.roomId = j.at("roomId").get<long>();
	r.userName = j.at("userName").get<string>();
	r.rtmpUrl = j.at("rtmpUrl").get<string>();
	//r.mpdUrl = j.at("mpdUrl").get<string>();
	r.roomSate = j.at("roomSate").get<int>();
}
void to_json(json& j, const GetRoomInfoRsp& rsp) {
	j = json{ {"roomList", rsp.roomList}, {"errCode", rsp.errCode}, {"msg", rsp.msg} };
}
void from_json(const json& j, GetRoomInfoRsp& rsp) {
	rsp.roomList = j.at("roomList").get<vector<RoomInfo>>();
	rsp.errCode = j.at("errCode").get<int>();
	rsp.msg = j.at("msg").get<string>();
}



//初始化函数
LiveList::LiveList(Fl_Window* window) : Fl_Group(0, 0, window->w(), window->h()) {
	string bodyString = "";
	getRoomListFunc(bodyString);
	std::cout << "----------------------------  bodyString  ----------------------" << std::endl;
	std::cout <<  bodyString << std::endl;
	//假数据
	//string jsonString = R"({"roomList":[{"roomId":20001,"userName":"Lily","rtmpUrl":"rtmp://xxx/xx/xx","mpdUrl":"xxx/xxx/xxx.mp4","roomSate":0}],"errCode":0,"msg":"ok"})";
	json j = json::parse(bodyString);
	GetRoomInfoRsp rsp;
	from_json(j, rsp);
	std::cout << "----------------------------  parse bodyString  ----------------------"  << std::endl;
	std::cout << "errCode:" << rsp.errCode << std::endl;
	std::cout << "msg:" << rsp.msg << std::endl;
	roomIdBox = new Fl_Box(0, 0, 200, 80, ClientUtil::fl_str("房间号："));
	roomIdBox = new Fl_Box(200, 0, 200, 80, ClientUtil::fl_str("用户名："));
	controlBox = new Fl_Box(400, 0, 200, 80, ClientUtil::fl_str("操作："));
	for (int i = 0; i < rsp.roomList.size(); i++)
	{
		cout <<  rsp.roomList[i].roomId << "  " << rsp.roomList[i].userName << "  " << rsp.roomList[i].rtmpUrl /*<< "   " << rsp.roomList[i].mpdUrl */<< "   " << rsp.roomList[i].roomSate << endl;
		auto roomId = std::to_string(rsp.roomList[i].roomId);
		auto roomId_Box = new Fl_Box(0, 80*(i+1), 200, 80, ClientUtil::toString<long>(rsp.roomList[i].roomId));
		auto userId_Box = new Fl_Box(200, 80*(i+1), 200, 80, ClientUtil::toString<string>(rsp.roomList[i].userName));
		auto btnBox = new Fl_Button(400, 80*(i+1), 200, 80, "enter");
		EnterInfo* enterInfo = new EnterInfo{ rsp.roomList[i].roomId, rsp.roomList[i].userName, rsp.roomList[i].rtmpUrl/*,rsp.roomList[i].mpdUrl*/,rsp.roomList[i].roomSate, window };
		//EnterInfo* enterInfo = new EnterInfo{ rsp.roomList[i].roomId, rsp.roomList[i].userName, "F:/Anime/mini yuri/tt.mkv"/*,rsp.roomList[i].mpdUrl*/,rsp.roomList[i].roomSate, window };
		btnBox->callback((Fl_Callback*)switchPage2, enterInfo);
	}
	this->end();
}

void LiveList::getRoomListFunc(string& bodyString) {
	std::shared_ptr<httplib::Response> rsp;
	HttpUtil Http;
	Http.getRequest("/zues/room/getRoomList", rsp);
	if (rsp) {
		if (rsp->status == 200) {
			std::cout << rsp->status << std::endl;
			std::cout << rsp->body << std::endl;
			bodyString = rsp->body;
		}
		else {
			std::cout << "request error" << std::endl;
		}
	}
	else {
		std::cout << "rsp is null" << std::endl;
	}

}

void LiveList::switchPage1(Fl_Button* btn, Fl_Window* window) {
	window->clear();
	window->begin();
	Fl_Group* group = new LoginPage(window);
	window->end();
	window->redraw();
}

void LiveList::switchPage2(Fl_Button* btn, EnterInfo* enterInfo) {
	enterInfo->window->clear();
	enterInfo->window->begin();
	Fl_Group* group = new LivePage(enterInfo->window, LiveInfo{enterInfo->roomId, enterInfo->userName, enterInfo->rtmpUrl});
	enterInfo->window->end();
	enterInfo->window->redraw();
}


