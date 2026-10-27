#include "crawler.hpp"
#include "utlis.hpp"
#include <mutex>
#include <stdexcept>

crawler::crawler()
	:slavesPool(20) 
{
	crawlerDBWrite.open("crawler.db", std::ios::app);
	if (!crawlerDBWrite.is_open())
		throw std::runtime_error("Failed to open crawler DB for writing");

	crawlerDBRead.open("crawler.db");
	if (!crawlerDBRead.is_open())
		throw std::runtime_error("Failed to open crawler DB for reading");


	for(auto i {0uz}; i < 20; ++i)
	{
		slavesPool[i] = new slave(utlis::slave_work, taskQ,
				taskMtx, results, resMtx,
				extractedLinks, extractedLinks_Mtx,
				cv);
	}
}

void crawler::crawl()
{
	while(true)
	{
		this->get_links_db();
		cv.notify_all();
		//make this thread sleep until taskQ is empty
		std::unique_lock<std::mutex> lk(taskMtx);
		cv.wait(lk, [this] {return taskQ.empty(); });
		lk.unlock();
		
		this->write_links_db();
		
	}
}

void crawler::get_links_db()
{
	constexpr size_t BATCH = 100;
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
	//	if (known.insert(link).second)    // true only if it wasn't there before
			crawlerDBWrite << link << '\n';

	crawlerDBWrite.flush();
}

crawler::~crawler()
{
	
}
