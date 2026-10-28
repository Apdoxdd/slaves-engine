
#pragma once

#include "slave.hpp"
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <vector>
#include <set>
#include <atomic>
#include <iostream>
#include <sstream>

namespace utlis 
{
	inline std::atomic<unsigned int> count {0};
	void setup_curl(CURL *&handle, std::string& response);
	bool do_request(CURL* handle, std::string& base_url, std::string& response);
	std::set<std::string> extract_links(std::string& base_url, std::string& response);
	void slave_work(CURL *&handle,
		std::queue<std::string>&,
		std::mutex&,
		std::vector<html_urls>&,
		std::mutex&,
		std::vector<std::string>&,
		std::mutex&,
		std::condition_variable&);
}
