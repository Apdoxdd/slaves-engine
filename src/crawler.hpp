#pragma once

#include "slave.hpp"

class crawler 
{
private:
	std::queue<std::string> taskQ;
	std::vector<html_urls> results;
	std::condition_variable cv;
	std::mutex urlMutex;
	std::mutex resMutex;
	std::vector<slave*> slavesPool;
public:
	crawler();
	~crawler();
	slave::fnWork doWork;
};



