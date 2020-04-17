#pragma once
#include "httplib.h"


class HttpUtil {
	
public:
	HttpUtil();
	
	void getRequest(const std::string& url, std::shared_ptr<httplib::Response>& rst);
	void postRequest(const std::string& url, const std::string& data, const std::string& header, std::shared_ptr<httplib::Response>& rst);
	void postRequest(const std::string& url, const httplib::Params& params, std::shared_ptr<httplib::Response>& rst);
private:
	httplib::Client* client;

};