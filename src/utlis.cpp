#include "utlis.hpp"

void utlis::setup_curl(CURL *handle, std::string& response)
{
	auto chunk_inc = [](char *chunk, size_t size, size_t nmemb, void *userp)
	{
		size_t bytes = size * nmemb;
		std::string *full_chunk = static_cast<std::string *>(userp);
		full_chunk->append(chunk, bytes);
		return bytes;
	};
	handle = curl_easy_init();
	if (!handle)
	{
		throw std::runtime_error{"Failed to create handle"};
	}
	curl_easy_setopt(handle, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA); 
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, +chunk_inc);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response);
	curl_easy_setopt(handle, CURLOPT_TIMEOUT, 300L);
	curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L);
}

bool utlis::do_request(CURL *handle, std::string& base_url, std::string& response)
{
	response.clear();
	curl_easy_setopt(handle, CURLOPT_URL, base_url.c_str());
	auto code = curl_easy_perform(handle);
	if (code != CURLE_OK)
		return false;
	else 
		return true;

}

std::set<std::string> utlis::extract_links(std::string& response)
{

}

void utlis::slave_work(CURL *handle,
		std::queue<std::string>& taskQ,
		std::mutex& taskMtx,
		std::vector<html_urls>& resutls,
		std::mutex& resMtx ,
		std::vector<std::string>& extractedLinks,
		std::mutex& extractedLinks_Mtx,
		std::condition_variable& cv)
{

}

