
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
#include <unordered_map>
#include <vector>
#include <set>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

 //bool normalize_link(const CURLU *base, std::string href, std::string& out);
namespace utlis 
{

	inline std::atomic<unsigned int> crawl_count {0};
	struct sheep 
	{
		std::string doc;
		std::string url;
		CURL *es_handle;
		bool active;
	};
	constexpr int MAX_SHEEP = 200;
	constexpr int LOCAL_Q_SIZE = 1000;

	inline std::atomic<unsigned int> count {0};
	void setup_multihandle(CURLM *&shephard_handle, std::unordered_map<CURL *, sheep>& sheep_map);
	void setup_easyhandle(CURL *&handle, std::string& response);
	bool do_request(CURL* handle, std::string& base_url, std::string& response);
	void extract_links(std::string& base_url, std::string& response, std::set<std::string>& links);
	void slave_work(CURLM *&shephard_handle,
		std::queue<std::string>&,
		std::mutex&,
		std::vector<html_urls>&,
		std::mutex&,
		std::vector<std::string>&,
		std::mutex&,
		std::condition_variable&);
}
