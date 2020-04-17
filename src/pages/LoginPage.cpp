//在这里制定事件响应函数

#include "pcClient/HttpUtil.h"
#include <iostream>
#include "pcClient/pages/LiveList.h"
#include "pcClient/pages/LoginPage.h"
#include "pcClient/pages/LivePage.h"
#include "pcClient/ClientUtil.h"

//初始化函数
LoginPage::LoginPage(Fl_Window* window) : Fl_Group(0, 0, window->w(), window->h()) {
//	btn1 = new Fl_Button(870, 500, 80, 30, ClientUtil::fl_str("发送"));
	box1 = new Fl_Box(10, 10, 940, 50, "Login page");
	Fl_Input* name = new Fl_Input(400, 125, 150, 30, ClientUtil::fl_str("用户名："));
	Fl_Secret_Input* pwd = new Fl_Secret_Input(400, 175, 150, 30, ClientUtil::fl_str("密码："));
	Info* info = new Info{ name, pwd, window };
	confirmBtn = new Fl_Button(425, 250, 80, 30, ClientUtil::fl_str("登录"));
	confirmBtn->callback((Fl_Callback*)switchPage, info);
//	confirmBtn->callback((Fl_Callback*)cb_copy, this);
//	btn1->callback((Fl_Callback*)switchPage, window);
	this->end();
}
//void LoginPage::cb_copy(Fl_Widget* ,void* v) {
//	((LoginPage*)v)->login();

//}
//void LoginPage::login() {
	//std::string username = name->value();
	//std::string  password = pwd->value();
//	if (username.empty() || password.empty()) {

//	}
//	else {

//		confirmBtn -> callback((Fl_Callback*)switchPage,window );
//		std::cout << username << std::endl;
//		std::cout << password << std::endl;
//	};

void LoginPage::switchPage(Fl_Button* btn, Info* info) {

	std::string username = info->name->value();
	std::string  password = info->pwd->value();

	if (username.empty() || password.empty()) {

	}
	else {
		std::cout << username << std::endl;
		std::cout << password << std::endl;
		//todo json
	    std::string str1 = "{\"userName\":\"";
		std::string name = username;
		str1 += name;
		std::string str2 = "\",\"userPasssword\":\"";
		str1 += str2;
		std::string pd = password;
		str1 += pd;
		std::string str3 = "\"}";
		str1 += str3;
		//test 
		std::shared_ptr<httplib::Response> rsp;
		HttpUtil Http;
		Http.postRequest("/zues/room/getRoomList", str1, "application/json", rsp);
		if (rsp) {
			if (rsp->status == 200) {
				std::cout  << rsp->status << std::endl;
				std::cout  << rsp->body << std::endl;
			}
			else {
				std::cout << "request error" << std::endl;
			}
		}
		else {
			std::cout << "rsp is null" << std::endl;
		}

		Http.getRequest("/zues/room/getRoomList", rsp);
		if (rsp) {
			if (rsp->status == 200) {
				std::cout <<  rsp->status << std::endl;
				std::cout <<  rsp->body << std::endl;
			}
			else {
				std::cout << "request error" << std::endl;
			}
		}
		else {
			std::cout << "rsp is null" << std::endl;
		}
	}



	info->window->clear();
	info->window->begin();
	Fl_Group* group = new LiveList(info->window);
	info->window->end();
	info->window->redraw();
}

