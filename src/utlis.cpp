#include "utlis.hpp"
#include <stdexcept>
#include <thread>

void setup_multihandle(CURLM *shephard_handle, std::unordered_map<CURL *, utlis::sheep> sheep_map)
{
	shephard_handle = curl_multi_init();
    	if(!shephard_handle) 
    	{
        	std::cerr << "curl_multi_init failed\n";
        	curl_global_cleanup();
		throw std::runtime_error("Failed to init multi handle");
    	}
    	curl_multi_setopt(shephard_handle, CURLMOPT_MAX_TOTAL_CONNECTIONS, static_cast<long>(utlis::MAX_SHEEP));

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

std::set<std::string> utlis:: extract_links(std::string& base_url, std::string& response)
{
    std::set<std::string> links;
    htmlDocPtr doc = htmlReadMemory(
        response.c_str(),
        response.size(),
        base_url.c_str(),
        nullptr,
        HTML_PARSE_NOERROR);

    if (doc == nullptr)
    {
        return links;
    }

    xmlXPathContextPtr context = xmlXPathNewContext(doc);

    if (context == nullptr)
    {
        xmlFreeDoc(doc);
        return links;
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

    return links;
}

void utlis::slave_work(CURL *&handle,
		std::queue<std::string>& taskQ,
		std::mutex& taskMtx,
		std::vector<html_urls>& results,
		std::mutex& resMtx ,
		std::vector<std::string>& extractedLinks,
		std::mutex& extractedLinks_Mtx,
		std::condition_variable& cv)
{
	std::string response {};
	setup_easyhandle(handle, response);
	auto id = std::this_thread::get_id();
        std::ostringstream oss;
        oss << id;
        std::string ID = oss.str();
        size_t _id = std::hash<std::thread::id>{}(id) % 20;
	while(true)
	{
		std::unique_lock<std::mutex> lk(taskMtx);
		cv.wait(lk, [&](){return !taskQ.empty(); });
		std::string base_url = std::move(taskQ.front());
		taskQ.pop();
		bool drained = taskQ.empty();
		lk.unlock();

		if (drained)
			cv.notify_all();

		if (base_url.empty())
		{
			curl_easy_cleanup(handle);
			handle = nullptr;
			return;
		}

		if(!do_request(handle, base_url, response))
			continue;

		utlis::count++;

//		std::cout<<"Count: "<<count<<" Thread ID: "<<_id<<std::endl;

		char *eff {nullptr};
		curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &eff);
		std::string final_url = eff? eff : base_url;
		

		std::set<std::string> links = extract_links(final_url, response);
		html_urls urls_html {base_url, std::move(response)};

		{
			std::lock_guard<std::mutex> resLk(resMtx);
			results.push_back(std::move(urls_html));
		}

		{
			std::lock_guard<std::mutex> linkLk(extractedLinks_Mtx);
			for (const auto &link : links)
				extractedLinks.push_back(link);

		}

	}
}
