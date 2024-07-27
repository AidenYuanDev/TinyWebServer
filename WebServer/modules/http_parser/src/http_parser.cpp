// http_parser.cpp

#include "http_parser.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>

std::string_view toString(HttpMethod method) {
    static constexpr std::array<std::string_view, 10> methodStrings = {
        "GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "TRACE", "CONNECT", "PATCH", "UNKNOWN"
    };
    return methodStrings[static_cast<int>(method)];
}

std::string_view toString(HttpVersion version) {
    static constexpr std::array<std::string_view, 4> versionStrings = {
        "HTTP/1.0", "HTTP/1.1", "HTTP/2.0", "UNKNOWN"
    };
    return versionStrings[static_cast<int>(version)];
}

HttpRequest::HttpRequest() : method(HttpMethod::UNKNOWN), version(HttpVersion::UNKNOWN) {}

HttpParser::HttpParser() {
    reset();
}

void HttpParser::reset() {
    state = ParserState::METHOD;
    request = HttpRequest{};
    currentHeaderName.clear();
    currentHeaderValue.clear();
    contentLength = 0;
}

std::optional<HttpRequest> HttpParser::parse(std::string_view data) {
    for (char c : data) {
        if (!parseChar(c)) {
            if (errorCallback) {
                errorCallback("Parsing error");
            }
            return std::nullopt;
        }
        if (state == ParserState::COMPLETE) {
            auto result = std::move(request);
            reset();
            return result;
        }
    }
    return std::nullopt;
}

void HttpParser::setErrorCallback(ErrorCallback cb) {
    errorCallback = std::move(cb);
}

bool HttpParser::parseChar(char c) {
    switch (state) {
        case ParserState::METHOD: return parseMethod(c);
        case ParserState::URL: return parseUrl(c);
        case ParserState::VERSION: return parseVersion(c);
        case ParserState::HEADER_NAME: return parseHeaderName(c);
        case ParserState::HEADER_VALUE: return parseHeaderValue(c);
        case ParserState::BODY: return parseBody(c);
        default: return false;
    }
}

bool HttpParser::parseMethod(char c) {
    if (c == ' ') {
        request.method = stringToMethod(request.url);
        request.url.clear();
        state = ParserState::URL;
    } else {
        request.url += c;
    }
    return true;
}

bool HttpParser::parseUrl(char c) {
    if (c == ' ') {
        state = ParserState::VERSION;
    } else {
        request.url += c;
    }
    return true;
}

bool HttpParser::parseVersion(char c) {
    static const std::string_view httpVersion = "HTTP/";
    static size_t versionIndex = 0;

    if (versionIndex < httpVersion.length()) {
        if (c == httpVersion[versionIndex]) {
            ++versionIndex;
        } else {
            return false;
        }
    } else if (c == '1' || c == '2') {
        request.version = (c == '1') ? HttpVersion::HTTP_1_1 : HttpVersion::HTTP_2_0;
        versionIndex = 0;
        state = ParserState::HEADER_NAME;
    } else {
        return false;
    }
    return true;
}

bool HttpParser::parseHeaderName(char c) {
    if (c == ':') {
        state = ParserState::HEADER_VALUE;
    } else if (c == '\r') {
        // Skip carriage return
    } else if (c == '\n') {
        if (!currentHeaderName.empty()) {
            processHeader();
        } else {
            state = (contentLength > 0) ? ParserState::BODY : ParserState::COMPLETE;
        }
    } else {
        currentHeaderName += std::tolower(c);
    }
    return true;
}

bool HttpParser::parseHeaderValue(char c) {
    if (c == '\r') {
        // Skip carriage return
    } else if (c == '\n') {
        processHeader();
        state = ParserState::HEADER_NAME;
    } else {
        currentHeaderValue += c;
    }
    return true;
}

bool HttpParser::parseBody(char c) {
    request.body.push_back(c);
    if (request.body.size() == contentLength) {
        state = ParserState::COMPLETE;
    }
    return true;
}

void HttpParser::processHeader() {
    if (currentHeaderName == "content-length") {
        contentLength = std::stoul(currentHeaderValue);
    }
    request.headers[currentHeaderName] = trim(currentHeaderValue);
    currentHeaderName.clear();
    currentHeaderValue.clear();
}

HttpMethod HttpParser::stringToMethod(std::string_view method) {
    static const std::unordered_map<std::string_view, HttpMethod> methodMap = {
        {"GET", HttpMethod::GET}, {"POST", HttpMethod::POST}, {"PUT", HttpMethod::PUT},
        {"DELETE", HttpMethod::DELETE}, {"HEAD", HttpMethod::HEAD}, {"OPTIONS", HttpMethod::OPTIONS},
        {"TRACE", HttpMethod::TRACE}, {"CONNECT", HttpMethod::CONNECT}, {"PATCH", HttpMethod::PATCH}
    };
    auto it = methodMap.find(method);
    return (it != methodMap.end()) ? it->second : HttpMethod::UNKNOWN;
}

std::string HttpParser::trim(const std::string& s) {
    auto start = std::find_if_not(s.begin(), s.end(), ::isspace);
    auto end = std::find_if_not(s.rbegin(), s.rend(), ::isspace).base();
    return (start < end) ? std::string(start, end) : std::string();
}

std::unordered_map<std::string, std::string> HttpParser::parseQueryParams(const std::string& url) {
    std::unordered_map<std::string, std::string> params;
    size_t pos = url.find('?');
    if (pos != std::string::npos) {
        std::string query = url.substr(pos + 1);
        std::istringstream iss(query);
        std::string pair;
        while (std::getline(iss, pair, '&')) {
            size_t eq_pos = pair.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = pair.substr(0, eq_pos);
                std::string value = pair.substr(eq_pos + 1);
                params[key] = value;
            }
        }
    }
    return params;
}