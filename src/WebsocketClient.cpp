#include "WebsocketClient.h"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/json.hpp>

#include <iostream>

namespace {
    constexpr auto defaultHost = "api.mollybet.com";
    constexpr auto defaultPort = 443;
    constexpr auto defaultTimeout = 30;
    constexpr auto streamURI = "/v1/stream/";
}// namespace

WebsocketClient::WebsocketClient::WebsocketClient(boost::asio::ssl::context &ctx) : ctx_{ctx} {}

boost::asio::awaitable<std::set<std::string>> WebsocketClient::getCompetitionName(const std::string &token) {
    auto resolver = boost::asio::use_awaitable_t<boost::asio::any_io_executor>::as_default_on(
            boost::asio::ip::tcp::resolver(co_await boost::asio::this_coro::executor));

    using executor_with_default = boost::asio::use_awaitable_t<>::executor_with_default<boost::asio::any_io_executor>;
    using tcp_stream = typename boost::beast::tcp_stream::rebind_executor<executor_with_default>::other;

    boost::beast::websocket::stream<boost::beast::ssl_stream<tcp_stream>> stream{
            boost::asio::use_awaitable_t<boost::asio::any_io_executor>::as_default_on(
                    boost::beast::tcp_stream(co_await boost::asio::this_coro::executor)),
            ctx_};

    if (!SSL_set_tlsext_host_name(stream.next_layer().native_handle(), defaultHost))
        throw boost::system::system_error(static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category());

    auto const results = co_await resolver.async_resolve(defaultHost, std::to_string(defaultPort));

    boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(defaultTimeout));

    co_await boost::beast::get_lowest_layer(stream).async_connect(results);

    boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(defaultTimeout));

    co_await stream.next_layer().async_handshake(boost::asio::ssl::stream_base::client);

    boost::beast::get_lowest_layer(stream).expires_never();

    stream.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));

    co_await stream.async_handshake(std::string(defaultHost) + ":" + std::to_string(defaultPort),
                                    std::string(streamURI) + "/?token=" + token, boost::asio::use_awaitable);

    std::set<std::string> competitionNames;
    while (true) {
        boost::beast::flat_buffer buffer;
        auto [ec, bytes] = co_await stream.async_read(buffer, as_tuple(boost::asio::use_awaitable));

        if (ec) {
            if (ec != boost::asio::error::eof) throw boost::beast::system_error(ec);
        }

        const auto body = boost::beast::buffers_to_string(buffer.data());
        const auto json = boost::json::parse(body);
        const auto data{json.as_object().if_contains("data")};
        if (data == nullptr || !data->is_array()) {
            std::cerr << "Failed to parse (\"data\" object is missed or of unexpected type): " << body << '\n';
        }

        for (const auto &i: data->get_array()) {
            if (!i.is_array()) { std::cerr << "Failed to parse (\"data\" of unexpected type): " << body << '\n'; }
            for (const auto &j: i.get_array()) {
                if (j.is_object()) {
                    const auto competitionName = j.get_object().if_contains("competition_name");
                    if (competitionName != nullptr) { competitionNames.emplace(competitionName->get_string()); }
                }
                if (j.is_string() && j == "sync") {
                    co_await stream.async_close(boost::beast::websocket::close_code::normal,
                                                boost::asio::use_awaitable);

                    co_return competitionNames;
                }
            }
        }
    }
}
