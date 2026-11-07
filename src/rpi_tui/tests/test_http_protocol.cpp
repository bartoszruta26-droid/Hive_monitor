#include "http_protocol.h"

#include <cassert>
#include <iostream>
#include <string>

using apiary::HttpRoute;
using apiary::resolveHttpRequestLine;

int main() {
    auto result = resolveHttpRequestLine("GET /health HTTP/1.1");
    assert(result.route == HttpRoute::Health && result.statusCode == 200);

    result = resolveHttpRequestLine("GET /api/hives?source=mobile HTTP/1.1");
    assert(result.route == HttpRoute::Status && result.statusCode == 200);

    result = resolveHttpRequestLine("GET /api/status HTTP/1.0");
    assert(result.route == HttpRoute::Status && result.statusCode == 200);

    result = resolveHttpRequestLine("GET /health/extra HTTP/1.1");
    assert(result.route == HttpRoute::NotFound && result.statusCode == 404);

    result = resolveHttpRequestLine("GET /healthXYZ HTTP/1.1");
    assert(result.route == HttpRoute::NotFound && result.statusCode == 404);

    result = resolveHttpRequestLine("POST /api/hives HTTP/1.1");
    assert(result.route == HttpRoute::MethodNotAllowed && result.statusCode == 405);

    result = resolveHttpRequestLine("HEAD /health HTTP/1.1");
    assert(result.route == HttpRoute::MethodNotAllowed && result.statusCode == 405);

    result = resolveHttpRequestLine("POST /x GET /health HTTP/1.1");
    assert(result.route == HttpRoute::BadRequest && result.statusCode == 400);

    result = resolveHttpRequestLine("GET /health HTTP/2");
    assert(result.route == HttpRoute::BadRequest && result.statusCode == 400);

    result = resolveHttpRequestLine("GET health HTTP/1.1");
    assert(result.route == HttpRoute::BadRequest && result.statusCode == 400);

    result = resolveHttpRequestLine("GET /health HTTP/1.1 extra");
    assert(result.route == HttpRoute::BadRequest && result.statusCode == 400);

    result = resolveHttpRequestLine("GET /api/sensors HTTP/1.1");
    assert(result.route == HttpRoute::Sensors && result.statusCode == 501);

    result = resolveHttpRequestLine("GET /api/effectors HTTP/1.1");
    assert(result.route == HttpRoute::Effectors && result.statusCode == 501);

    result = resolveHttpRequestLine("GET /api/history HTTP/1.1");
    assert(result.route == HttpRoute::History && result.statusCode == 501);

    result = resolveHttpRequestLine("GET /api/logs HTTP/1.1");
    assert(result.route == HttpRoute::Logs && result.statusCode == 501);
    assert(std::string(apiary::httpReasonPhrase(501)) == "Not Implemented");
    assert(std::string(apiary::httpReasonPhrase(503)) == "Service Unavailable");

    std::cout << "HTTP request-line routing tests passed\n";
    return 0;
}
