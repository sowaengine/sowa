#include "net.hpp"
#include <curl/curl.h>
#include <iostream>

size_t get_data(char *buf, size_t itemSize, size_t nItems, void *userData) {
	size_t bytes = itemSize * nItems;
	if (userData)
		reinterpret_cast<std::string *>(userData)->append(buf);
	return bytes;
}

std::string GetFileHTTP(const std::string &url) {
	CURL *curl = curl_easy_init();
	if (!curl) {
		std::cerr << "Failed to initialize curl\n";
		exit(1);
	}
	std::string buffer = "";

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_data);

	CURLcode result = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if (result != CURLE_OK) {
		std::cerr << "Failed to download version info: " << curl_easy_strerror(result) << "\n";
		return "";
	}

	return buffer;
}