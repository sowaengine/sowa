#ifndef SW_RUNNER_NET_HPP_
#define SW_RUNNER_NET_HPP_

#include <string>
#include <functional>

std::string GetContentHTTP(const std::string& url);
std::vector<unsigned char> GetFileHTTP(const std::string& url);

#endif // SW_RUNNER_NET_HPP_