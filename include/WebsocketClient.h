#pragma once

#include <boost/asio/awaitable.hpp>
#include <boost/asio/ssl/context.hpp>

#include <set>
#include <string>

class WebsocketClient {
public:
    explicit WebsocketClient(boost::asio::ssl::context &ctx);

    boost::asio::awaitable<std::set<std::string>> getCompetitionName(const std::string &token);

private:
    boost::asio::ssl::context &ctx_;
};
