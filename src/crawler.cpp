#include "crawler.hpp"
#include "utlis.hpp"
#include <mutex>
#include <stdexcept>

crawler::crawler()
	:slavesPool(1) 
{
	if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
    	{
		throw::std::runtime_error("CURL global init failed");
   	 }
	crawlerDBWrite.open("../crawler.db", std::ios::app);
	if (!crawlerDBWrite.is_open())
		throw std::runtime_error("Failed to open crawler DB for writing");

	crawlerDBRead.open("../crawler.db");
	if (!crawlerDBRead.is_open())
		throw std::runtime_error("Failed to open crawler DB for reading");


	for(auto i {0uz}; i < slavesPool.size(); ++i)
	{
		slavesPool[i] = new slave(utlis::slave_work, taskQ,
				taskMtx, results, resMtx,
				extractedLinks, extractedLinks_Mtx,
				cv);
	}
}

void crawler::crawl()
{
	auto start = std::chrono::steady_clock::now();
	auto next_print {1000};
	auto batches {0};
	while(true)
	{

		this->get_links_db();
		cv.notify_all();
		//make this thread sleep until taskQ is empty
		std::unique_lock<std::mutex> lk(taskMtx);
		cv.wait(lk, [this] {return taskQ.empty(); });
		lk.unlock();
		
		this->write_links_db();

		batches++;
        	if (utlis::crawl_count >= next_print)
        	{
        		double mins = std::chrono::duration<double>(
                          std::chrono::steady_clock::now() - start).count() / 60.0;
        		std::cout << utlis::crawl_count / mins << " pages/min ("
                  	<< utlis::crawl_count << " pages)(" << batches << ")\n";
        
        		next_print += 1000;
       		 }


	}
}

void crawler::get_links_db()
{
	constexpr size_t BATCH = 2000;
	crawlerDBRead.clear();

	std::vector<std::string> batch;
	std::string line;

	while(batch.size() < BATCH && std::getline(crawlerDBRead, line))
	{
		if (!line.empty())
			batch.push_back(std::move(line));

	}
	if (batch.empty())
		return;

	{
		std::lock_guard<std::mutex> lk(taskMtx);
		for (auto& url : batch)
			taskQ.push(std::move(url));
	}

}

void crawler::write_links_db()
{
	std::vector<std::string> links;
	{
		std::lock_guard<std::mutex> lk(extractedLinks_Mtx);
		links.swap(extractedLinks);   // take everything, keep the lock short
	}
		for (auto& link : links)
			if (seen.insert(link).second)    // true only if it wasn't there before
				crawlerDBWrite << link << '\n';

	crawlerDBWrite.flush();
}

crawler::~crawler()
{

	for(auto i {0uz}; i < slavesPool.size(); ++i)
		delete slavesPool[i];

}
