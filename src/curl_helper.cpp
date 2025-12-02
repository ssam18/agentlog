#include "curl_helper.h"
#include <cstring>

namespace agentlog {

CurlHelper::CurlHelper() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_ = curl_easy_init();
}

CurlHelper::~CurlHelper() {
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
    curl_global_cleanup();
}

size_t CurlHelper::write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), total_size);
    return total_size;
}

CurlHelper::Response CurlHelper::post(const std::string& url,
                                       const std::string& json_body,
                                       const std::map<std::string, std::string>& headers) {
    Response response;
    
    if (!curl_) {
        return response;
    }

    // Set URL
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    
    // Set POST method
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    
    // Set POST data
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, json_body.c_str());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, json_body.length());
    
    // Set headers
    struct curl_slist* header_list = nullptr;
    header_list = curl_slist_append(header_list, "Content-Type: application/json");
    
    for (const auto& [key, value] : headers) {
        std::string header = key + ": " + value;
        header_list = curl_slist_append(header_list, header.c_str());
    }
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, header_list);
    
    // Set write callback to capture response
    std::string response_body;
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_body);
    
    // Set timeouts
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 10L);
    
    // Enable SSL verification
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2L);
    
    // Perform request
    CURLcode res = curl_easy_perform(curl_);
    
    // Cleanup headers
    curl_slist_free_all(header_list);
    
    if (res == CURLE_OK) {
        long http_code = 0;
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
        response.status_code = static_cast<int>(http_code);
        response.body = response_body;
        response.success = (http_code >= 200 && http_code < 300);
    } else {
        response.success = false;
        response.body = curl_easy_strerror(res);
    }
    
    return response;
}

} // namespace agentlog
