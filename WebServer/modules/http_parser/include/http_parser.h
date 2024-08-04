// modules/http_parser/include/http_parser.h

#pragma once

#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "logger.h"

enum class HttpMethod { GET, HEAD, POST, PUT, DELETE, CONNECT, OPTIONS, TRACE, PATCH, UNKNOWN };
enum class HttpVersion { HTTP_1_0, HTTP_1_1, HTTP_2_0, UNKNOWN };

struct HttpRequest {
    HttpMethod method = HttpMethod::UNKNOWN;
    std::string url;
    HttpVersion version = HttpVersion::UNKNOWN;
    std::unordered_map<std::string, std::string> headers;
    std::vector<char> body;

    HttpRequest();
};

class IHttpParser {
public:
    using ErrorCallback = std::function<void(const std::string &)>;

    virtual ~IHttpParser() = default;
    virtual void reset() = 0;
    virtual std::optional<HttpRequest> parse(std::string_view data) = 0;
    virtual void setErrorCallback(ErrorCallback cb) = 0;
};

class HttpParser : public IHttpParser {
public:
    HttpParser();
    void reset() override;
    std::optional<HttpRequest> parse(std::string_view data) override;
    void setErrorCallback(ErrorCallback cb) override;

    static std::unordered_map<std::string, std::string> parseQueryParams(const std::string &url);

private:
    enum class ParserState { METHOD, URL, VERSION, HEADER_NAME, HEADER_VALUE, BODY, COMPLETE, ERROR };

    ParserState state;
    HttpRequest request;
    std::string currentHeaderName;
    std::string currentHeaderValue;
    size_t contentLength;
    ErrorCallback errorCallback;

    bool parseChar(char c);
    bool parseMethod(char c);
    bool parseUrl(char c);
    bool parseVersion(char c);
    bool parseHeaderName(char c);
    bool parseHeaderValue(char c);
    bool parseBody(char c);
    void processHeader();
    static HttpMethod stringToMethod(std::string_view method);
    static std::string trim(const std::string &s);
};

std::string_view toString(HttpMethod method);
std::string_view toString(HttpVersion version);
