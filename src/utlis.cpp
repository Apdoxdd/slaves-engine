#include "utlis.hpp"

void utlis::setup_curl(CURL *&handle, std::string& response)
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
	curl_easy_setopt(handle, CURLOPT_TIMEOUT, 300L); // setup 5 mins error
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

        for (int i = 0; i < nodes->nodeNr; ++i)
        {
            xmlChar *href = xmlNodeGetContent(nodes->nodeTab[i]);

            if (href)
            {
                xmlChar *absolute_url = xmlBuildURI(href, BAD_CAST base_url.c_str()); // بتشوف لو الرابط مفروض يتنسب للبيز لينك وتربطه

                if (absolute_url)
                {
                    links.insert(
                        reinterpret_cast<const char *>(absolute_url));
                    xmlFree(absolute_url);
                }

                xmlFree(href);
            }
        }
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
	setup_curl(handle, response);
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
			return;

		if(!do_request(handle, base_url, response))
			continue;

		utlis::count++;

		std::set<std::string> links = extract_links(base_url, response);
		html_urls urls_html {base_url, std::move(response)};

		{
			std::lock_guard<std::mutex> resLk(resMtx);
			results.push_back(std::move(urls_html));
		}

		{
			std::lock_guard<std::mutex> linkLk(extractedLinks_Mtx);
			for (auto link : links)
				extractedLinks.push_back(link);

		}

	}
}

