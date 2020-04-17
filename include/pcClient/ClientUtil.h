#pragma once

#include <atlstr.h>
#include <string>
#include <iostream>
#include <stdio.h>
#include <sstream>

namespace ClientUtil {

    char* fl_str(char* cstr);
    char* fl_cstr(const char* cstr);
    template<typename T> char* toString(const T& t) {
        std::stringstream ss{};
        ss << t ;
        return fl_cstr(ss.str().c_str());
    }

}