#include "pcClient/HttpUtil.h"
#include <iostream>
//#include "constant.h"


HttpUtil::HttpUtil() {
	//this->client = new httplib::Client(localhost, 1234);
	this->client = new httplib::Client("theia2.seekloud.org", 50080);
}

void HttpUtil::getRequest(const std::string& url, std::shared_ptr<httplib::Response>& rst) {
	auto res = this->client->Get(url.c_str());
	rst = res;
}

void HttpUtil::postRequest(const std::string& url, const std::string& data, const std::string& header, std::shared_ptr<httplib::Response>& rst) {
	auto res = this->client->Post(url.c_str(), data.c_str(), header.c_str());
	rst = res;
}

void HttpUtil::postRequest(const std::string& url, const httplib::Params& params, std::shared_ptr<httplib::Response>& rst) {
	auto res = this->client->Post(url.c_str(), params);
	rst = res;

}