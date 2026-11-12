#include "utlis.hpp"
#include "curl/curl.h"
#include <stdexcept>
#include <thread>
#include <unordered_map>

void utlis::setup_multihandle(CURLM *&shephard_handle, std::unordered_map<CURL *, utlis::sheep>& sheep_map)
{
	shephard_handle = curl_multi_init();
    	if(!shephard_handle) 
    	{
        	std::cerr << "curl_multi_init failed\n";
        	curl_global_cleanup();
		throw std::runtime_error("Failed to init multi handle");
    	}
    	curl_multi_setopt(shephard_handle, CURLMOPT_MAX_TOTAL_CONNECTIONS, static_cast<long>(utlis::MAX_SHEEP));

    	for (auto i {0}; i < utlis::MAX_SHEEP; ++i)
    	{
		utlis::sheep cur_sheep;
		utlis::setup_easyhandle(cur_sheep.es_handle, cur_sheep.doc);
		sheep_map[cur_sheep.es_handle] = cur_sheep;
    	}
}

void utlis::setup_easyhandle(CURL *&handle, std::string& response)
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
	curl_easy_setopt(handle, CURLOPT_USERAGENT, "MyCrawler/1.0");
	curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5L); // setup 5 secs error
	curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L); // fail on 400 requests
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 10L);
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

static bool is_login_path(const std::string &path)
{
	static const std::set<std::string> words = {
		"login", "log-in", "signin", "sign-in", "signup", "sign-up",
		"register", "logout", "log-out", "signout", "sign-out",
		"oauth", "auth", "sso", "account-login", "forgot-password"};

	size_t start = 0;
	while (start <= path.size())
	{
		size_t end = path.find('/', start);
		if (end == std::string::npos)
			end = path.size();
		std::string seg = path.substr(start, end - start);
		start = end + 1;

		std::transform(seg.begin(), seg.end(), seg.begin(),
				[](unsigned char c) { return std::tolower(c); });
		if (words.count(seg))
			return true;
	}
	return false;
}

static bool has_blocked_extension(const std::string &path)
{
	size_t slash = path.find_last_of('/');
	std::string last = path.substr(slash == std::string::npos ? 0 : slash + 1);
	size_t dot = last.find_last_of('.');
	if (dot == std::string::npos)
		return false;

	std::string ext = last.substr(dot + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return std::tolower(c); });

	static const std::set<std::string> blocked = {
		"jpg","jpeg","png","gif","svg","webp","ico","bmp","avif",
		"pdf","doc","docx","xls","xlsx","ppt","pptx",
		"zip","gz","tar","rar","7z","iso","exe","dmg","apk",
		"mp3","mp4","avi","mov","webm","mkv","wav",
		"css","js","json","woff","woff2","ttf","eot", "kh", "sa", "wa"};
	return blocked.count(ext) > 0;
}

static bool normalize_link(const CURLU *base, std::string href, std::string& out)
{
	auto first = href.find_first_not_of(" \t\r\n\f");
	if (first == std::string::npos)
		return false;
	auto last = href.find_last_not_of(" \t\r\n\f");
	href = href.substr(first, last - first +1);

	href.erase(std::remove_if(href.begin(), href.end(), 
				[](char c) {return c == '\t' || c == '\r' || c == '\n'; }), href.end());
	if (href.empty() || href[0] == '#')
		return false;

	// encode spaces to %20 so we dont relay on libcurl version
	for (size_t pos = 0; (pos = href.find(' ', pos)) != std::string::npos; pos +=3)
		href.replace(pos, 1, "%20");

	CURLU *u = curl_url_dup(base); //make a private copy of base
	if (!u)
		return false;

	char *scheme {nullptr};
	char *host {nullptr};
	char *result {nullptr};
	char *path {nullptr};
	bool ok {false};

	do 
	{
		// 2) resolve href against the base
		if (curl_url_set(u, CURLUPART_URL, href.c_str(), CURLU_DISALLOW_USER | CURLU_URLENCODE) != CURLUE_OK)
			break;

		if (curl_url_get(u, CURLUPART_PATH, &path, 0) != CURLUE_OK)
			break;
		std::string p = path;

		while (p.size() > 1 && p.back() == '/')
			p.pop_back();

		if (has_blocked_extension(p) || is_login_path(p))
			break;

		curl_url_set(u, CURLUPART_PATH, p.c_str(), 0);
		// 3) http or https
		if (curl_url_get(u, CURLUPART_SCHEME, &scheme, 0) != CURLUE_OK)
			break;
		if (std::strcmp(scheme, "http") != 0 && std::strcmp(scheme, "https") != 0)
			break;

		curl_url_set(u, CURLUPART_FRAGMENT, nullptr, 0);


		if (curl_url_get(u, CURLUPART_HOST, &host, 0) != CURLUE_OK)
			break;
		for (char *c = host; *c; ++c)
			if (*c >= 'A' && *c <= 'Z')
				*c += 'a' - 'A';
		size_t host_len = std::strlen(host);               // new
		while (host_len > 1 && host[host_len - 1] == '.')  // new
    			host[--host_len] = '\0';

		if (curl_url_set(u, CURLUPART_HOST, host, 0) != CURLUE_OK)
			break;

		if (curl_url_get(u, CURLUPART_URL, &result, 
					CURLU_NO_DEFAULT_PORT | CURLU_PUNYCODE) != CURLUE_OK)
			break;

		out = result;
		ok = true;
	}while (false);

	curl_free(scheme);
	curl_free(host);
	curl_free(result);
	curl_free(path);
	curl_url_cleanup(u);

	return ok;

	
}

void utlis:: extract_links(std::string& base_url, std::string& response, std::set<std::string>& links)
{
    htmlDocPtr doc = htmlReadMemory(
        response.c_str(),
        response.size(),
        base_url.c_str(),
        nullptr,
        HTML_PARSE_NOERROR);

    if (doc == nullptr)
    {
        return;
    }

    xmlXPathContextPtr context = xmlXPathNewContext(doc);

    if (context == nullptr)
    {
        xmlFreeDoc(doc);
        return;
    }

    xmlXPathObjectPtr result = xmlXPathEvalExpression(BAD_CAST "//a/@href", context);

    if (result && result->nodesetval)
    {
        xmlNodeSetPtr nodes = result->nodesetval;

        CURLU *base = curl_url();
        bool base_ok = curl_url_set(base, CURLUPART_URL, base_url.c_str(), 0) == CURLUE_OK;

        if (base_ok)
         for (int i = 0; i < nodes->nodeNr; ++i)
         {
            xmlChar *href = xmlNodeGetContent(nodes->nodeTab[i]);

            if (href)
            {
		    std::string out;
		    if (normalize_link(base, reinterpret_cast<const char *>(href), out))
		    	    links.insert(out);
		    xmlFree(href);
            }
         }
        curl_url_cleanup(base);
    }

    if (result)
        xmlXPathFreeObject(result);

    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);

}

void utlis::slave_work(CURLM *&shephard_handle,
		std::queue<std::string>& taskQ,
		std::mutex& taskMtx,
		std::vector<html_urls>& results,
		std::mutex& resMtx ,
		std::vector<std::string>& extractedLinks,
		std::mutex& extractedLinks_Mtx,
		std::condition_variable& cv)
{
	std::unordered_map<CURL *, utlis::sheep> sheep_map;
	setup_multihandle(shephard_handle, sheep_map);

	std::queue<std::string> local_taskQ;
	std::vector<html_urls> local_results;
	std::set<std::string> local_extracted_links;
	int running_sheep {0};

	while(true)
	{
		local_results.clear();
		local_extracted_links.clear();
		std::unique_lock<std::mutex> lk(taskMtx);
		cv.wait(lk, [&](){return !taskQ.empty(); });
		for (auto i {0}; i < utlis::LOCAL_Q_SIZE && !taskQ.empty(); ++i)
		{
			std::string task_url = std::move(taskQ.front());
			taskQ.pop();
			local_taskQ.push(task_url);

		}
		bool drained = taskQ.empty();
		if (drained)
			cv.notify_all();

		while (!local_taskQ.empty())
		{
			for (auto& [handle, cur_sheep] : sheep_map)
			{
				if (local_taskQ.empty())
					break;
				if (cur_sheep.active)
					continue;
				cur_sheep.url = local_taskQ.front();
				local_taskQ.pop();
				cur_sheep.doc.clear();

				CURLcode es_code = curl_easy_setopt(cur_sheep.es_handle, CURLOPT_URL, cur_sheep.url.c_str());
				if (es_code != CURLE_OK)
				{
					std::cerr << "curl_easy_setopt failed for task " <<std::endl;
					continue;
				}
				CURLMcode mul_code = curl_multi_add_handle(shephard_handle, cur_sheep.es_handle); 
				if (mul_code != CURLM_OK)
				{
					std::cerr<<"curl_multi_add_handle failed for task"<<std::endl;
					continue;
				}
				cur_sheep.active = true;
			}
			curl_multi_perform(shephard_handle, &running_sheep);

			int messagesLeft {0};
			CURLMsg *message {nullptr};


			CURL *finished_handle {nullptr};
			CURLcode msg_code;

			while ((message = curl_multi_info_read(shephard_handle, &messagesLeft)))
			{
				if (message->msg != CURLMSG_DONE)
					continue;

				finished_handle = message->easy_handle;
				msg_code = message->data.result;

				auto finished_sheep = sheep_map[finished_handle];

				if (msg_code == CURLE_OK)
				{
					long httpStatus {0};
					curl_easy_getinfo(finished_handle, CURLINFO_RESPONSE_CODE, &httpStatus);
					if (httpStatus >= 200 && httpStatus < 300)
					{
						crawl_count++;
						local_results.push_back({finished_sheep.url, finished_sheep.doc});
					}
				}
				curl_multi_remove_handle(shephard_handle, finished_handle);
				finished_sheep.active = false;

				

			}

			if (running_sheep == 0 && local_results.empty())
				break;
			
        		if(running_sheep > 0)
            		curl_multi_poll(shephard_handle, nullptr, 0, 1000, nullptr);

		}

		for (auto &doc : local_results)
		{
			extract_links(doc.url, doc.doc, local_extracted_links);
		}

		{
			std::lock_guard<std::mutex> resLk(resMtx);
			for ( auto &result : local_results)
				results.push_back(std::move(result));
		}

		{
			std::lock_guard<std::mutex> linkLk(extractedLinks_Mtx);
			for (auto &link : local_extracted_links)
				extractedLinks.push_back(link);
		}
		


	}
	 for (auto &[handle, sheep] : sheep_map)
    	{
        	if (sheep.active)
            		curl_multi_remove_handle(shephard_handle, handle);

        	curl_easy_cleanup(handle);
    	}
}


