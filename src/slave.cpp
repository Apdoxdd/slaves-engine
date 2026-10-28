#include "slave.hpp"

			
slave::slave(slave_work doWork, 
	std::queue<std::string>& taskQ,
	std::mutex& taskMtx,
	std::vector<html_urls>& results,
	std::mutex& resMtx,
	std::vector<std::string>& extractedLinks,
	std::mutex& extractedLinks_Mtx,
	std::condition_variable& cv)
					:
					worker(doWork, std::ref(handle), std::ref(taskQ), std::ref(taskMtx), std::ref(results), std::ref(resMtx), 
							std::ref(extractedLinks), std::ref(extractedLinks_Mtx), std::ref(cv))
					//crazy desgin decision ikr

{}

slave::~slave()
{
	if(worker.joinable())
		worker.join();
	curl_easy_cleanup(handle);
}
