

#include "crawler.hpp"
#include "utlis.hpp"

#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

#include <iostream>

// un comment the normalize link declaration in utlies http to use the test and m
// also remove the static from utlis cpp nromalize_links

static void t(const CURLU *base, const std::string &href)
{
   // std::string o;
   // bool ok = normalize_link(base, href, o);
    //std::cout << (ok ? "OK   " : "SKIP ") << "[" << href << "] -> " << (ok ? o : "") << "\n";
}
//  for testing purposes

int main ()
{
/*CURLU *base = curl_url();
    curl_url_set(base, CURLUPART_URL, "https://a.com/x/", 0);

    t(base, "my page?q=a%20b&c=é");
    t(base, "https://Example.com./p");
    t(base, "//CDN.Example.COM:443/a");
    t(base, "../y/z.html#frag");
    t(base, "mailto:me@a.com");
    t(base, "#top");
    t(base, "http://user:pw@a.com/");

    curl_url_cleanup(base);

    */

		curl_global_init(CURL_GLOBAL_DEFAULT);
	crawler c1;
	c1.crawl();
}

