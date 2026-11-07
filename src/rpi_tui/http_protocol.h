#pragma once

#include <sstream>
#include <string>

namespace apiary {

enum class HttpRoute {
    BadRequest,
    MethodNotAllowed,
    NotFound,
    Status,
    Health,
    Csv,
    Sensors,
    Effectors,
    History,
    Logs
};

struct HttpRouteResult {
    HttpRoute route;
    int statusCode;
};

// Parses only the HTTP request line. Exact path matching prevents prefix and
// embedded-substring matches such as "POST /x GET /health".
inline HttpRouteResult resolveHttpRequestLine(const std::string& requestLine) {
    std::istringstream input(requestLine);
    std::string method;
    std::string target;
    std::string version;
    std::string extra;

    if (!(input >> method >> target >> version) || (input >> extra)) {
        return {HttpRoute::BadRequest, 400};
    }
    if (version != "HTTP/1.0" && version != "HTTP/1.1") {
        return {HttpRoute::BadRequest, 400};
    }
    if (target.empty() || target.front() != '/' || target.find('#') != std::string::npos) {
        return {HttpRoute::BadRequest, 400};
    }
    const auto query = target.find('?');
    const std::string path = target.substr(0, query);

    if (method != "GET") {
        return {HttpRoute::MethodNotAllowed, 405};
    }

    if (path == "/api/status" || path == "/api/hives") return {HttpRoute::Status, 200};
    if (path == "/health") return {HttpRoute::Health, 200};
    if (path == "/api/csv") return {HttpRoute::Csv, 200};
    // These routes are reserved but have no live backend implementation yet.
    if (path == "/api/sensors") return {HttpRoute::Sensors, 501};
    if (path == "/api/effectors") return {HttpRoute::Effectors, 501};
    if (path == "/api/history") return {HttpRoute::History, 501};
    if (path == "/api/logs") return {HttpRoute::Logs, 501};
    return {HttpRoute::NotFound, 404};
}

inline const char* httpReasonPhrase(int statusCode) {
    switch (statusCode) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 501: return "Not Implemented";
        case 503: return "Service Unavailable";
        default: return "Internal Server Error";
    }
}

} // namespace apiary
