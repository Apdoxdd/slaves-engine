#include "slave.hpp"

slave::slave(fnWork doWork, std::queue<std::string> &taskQ, std::vector<html_urls> &results, std::condition_variable &cv,
			std::mutex &urlMtx, std::mutex &resMtx)
			
			: handle(curl_easy_init()),
			  worker(doWork, handle, std::ref(taskQ), std::ref(results), std::ref(cv), std::ref(urlMtx), 
			  		  std::ref(resMtx)) 
{
	if (!handle)
	{
		throw std::runtime_error("Failed to create handle");
	}
	curl_easy_setopt(this->handle, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA); 
}

slave::~slave()
{
	if(worker.joinable())
		worker.join();
	curl_easy_cleanup(handle);
}
