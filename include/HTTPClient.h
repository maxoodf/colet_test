#pragma once

#include <boost/asio/awaitable.hpp>
#include <boost/asio/ssl/context.hpp>

#include <string>

class HTTPClient {
public:
    explicit HTTPClient(boost::asio::ssl::context &ctx);

    boost::asio::awaitable<std::pair<uint, std::string>> getSessionToken();

private:
    boost::asio::ssl::context &ctx_;
};
