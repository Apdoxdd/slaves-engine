#pragma once
#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <stdexcept>

struct html_urls 
{
	std::string url;
	std::string doc;
};

class slave 
{
private:
	CURL *handle;
	std::thread worker;
public:
	using fnWork = void (*)(CURL *handle,
				std::queue<std::string>&,
				std::vector<html_urls>&,
				std::condition_variable&,
				std::mutex&, std::mutex&);
	slave(fnWork doWorkk, std::queue<std::string> &taskQ, std::vector<html_urls> &results, std::condition_variable &cv,
			std::mutex &urlMtx, std::mutex &resMtx);
	~slave();
};
			
