#pragma once

#include<FL/Fl.H>
#include<FL/Fl_Window.H>
#include<FL/Fl_Box.H>
#include<FL/Fl_Button.H>
#include<FL/Fl_Input.H>
#include <FL/Fl_Secret_Input.H>


struct Info {
	Fl_Input* name;
	Fl_Secret_Input* pwd;
	Fl_Window* window;
};


class LoginPage: public Fl_Group {
public:
	LoginPage(Fl_Window* window);
private:
	static void switchPage(Fl_Button* btn, Info*);
	Fl_Button* btn1, *confirmBtn ;
	Fl_Box* box1;
//	Fl_Input* name;
//	Fl_Secret_Input* pwd;
//	static void cb_copy(Fl_Widget* , void*);
//	inline void login();

};

