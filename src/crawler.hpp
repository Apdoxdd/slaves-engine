#pragma once

#include "slave.hpp"
#include <fstream>
#include <unordered_set>

class crawler 
{
private:
	std::queue<std::string> taskQ;
	std::mutex taskMtx;
	std::vector<html_urls> results;
	std::mutex resMtx;
	std::vector<slave*> slavesPool;
	std::vector<std::string> extractedLinks;
	std::mutex extractedLinks_Mtx;
	std::condition_variable cv;
	std::unordered_set<std::string> seen;


	std::ifstream crawlerDBRead;
	std::ofstream crawlerDBWrite;

public:
	crawler();
	~crawler();
	void crawl();
	void get_links_db();
	void write_links_db();

};



