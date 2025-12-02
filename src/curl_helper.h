#ifndef AGENTLOG_CURL_HELPER_H
#define AGENTLOG_CURL_HELPER_H

#include <string>
#include <map>
#include <curl/curl.h>

namespace agentlog {

// Helper class for HTTPS POST requests using libcurl
class CurlHelper {
public:
    struct Response {
        int status_code{0};
        std::string body;
        bool success{false};
    };

    CurlHelper();
    ~CurlHelper();

    // Perform HTTPS POST request
    Response post(const std::string& url, 
                  const std::string& json_body,
                  const std::map<std::string, std::string>& headers = {});

private:
    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);
    
    CURL* curl_{nullptr};
};

} // namespace agentlog

#endif // AGENTLOG_CURL_HELPER_H
