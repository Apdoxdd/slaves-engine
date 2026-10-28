

#include "crawler.hpp"

#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

#include <iostream>

int main ()
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
	crawler c1;
	c1.crawl();
}

