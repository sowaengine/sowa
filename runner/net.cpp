#include "net.hpp"
#include <curl/curl.h>
#include <iostream>
#include <fstream>

size_t get_text(char *buf, size_t itemSize, size_t nItems, void *userData) {
	size_t bytes = itemSize * nItems;
	if (userData)
		reinterpret_cast<std::string *>(userData)->append(buf);
	return bytes;
}

size_t get_file(char *buf, size_t itemSize, size_t nItems, FILE *userData) {
	size_t bytes = itemSize * nItems;

	if (userData) {
		auto& buffer = (*reinterpret_cast<std::vector<unsigned char> *>(userData));
		buffer.insert(buffer.end(), buf, buf+bytes);
	}
	return bytes;
}

std::string GetContentHTTP(const std::string &url) {
	CURL *curl = curl_easy_init();
	if (!curl) {
		std::cerr << "Failed to initialize curl\n";
		exit(1);
	}
	std::string buffer = "";

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_text);

	CURLcode result = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if (result != CURLE_OK) {
		std::cerr << "Failed to download version info: " << curl_easy_strerror(result) << "\n";
		return "";
	}

	return buffer;
}

std::vector<unsigned char> GetFileHTTP(const std::string &url) {
	CURL *curl = curl_easy_init();
	if (!curl) {
		std::cerr << "Failed to initialize curl\n";
		exit(1);
	}
	
	std::vector<unsigned char> buffer{};
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_file);

	CURLcode result = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if (result != CURLE_OK) {
		std::cerr << "Failed to download version info: " << curl_easy_strerror(result) << "\n";
		return {};
	}

	return buffer;
}