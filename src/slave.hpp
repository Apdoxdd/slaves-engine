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
	CURL *handle = nullptr;
	std::thread worker;
public:
	using slave_work = void (*)(CURL *&handle,
		std::queue<std::string>&,
		std::mutex&,
		std::vector<html_urls>&,
		std::mutex&,
		std::vector<std::string>&,
		std::mutex&,
		std::condition_variable&);

	slave(slave_work slave_work, 
		std::queue<std::string>& taskQ,
		std::mutex& taskMtx,
		std::vector<html_urls>& results,
		std::mutex& resMtx,
		std::vector<std::string>& extractedLinks,
		std::mutex& extractedLinks_Mtx,
		std::condition_variable& cv);

	~slave();
	friend class crawler;
};
			
