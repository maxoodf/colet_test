#include "HTTPClient.h"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/json.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace {
    constexpr auto defaultHost = "api.mollybet.com";
    constexpr auto defaultPort = 443;
    constexpr auto defaultHTTPVersion = 11;
    constexpr auto defaultTimeout = 30;
    constexpr auto loginURI = "/v1/sessions/";
    constexpr auto defaultUser = "devinterview";
    constexpr auto defaultPassword = "OwAb6wrocirEv";
}// namespace

HTTPClient::HTTPClient(boost::asio::ssl::context &ctx) : ctx_{ctx} {}

boost::asio::awaitable<std::pair<uint, std::string>> HTTPClient::getSessionToken() {
    auto resolver = boost::asio::use_awaitable_t<boost::asio::any_io_executor>::as_default_on(
            boost::asio::ip::tcp::resolver(co_await boost::asio::this_coro::executor));

    using executor_with_default = boost::asio::use_awaitable_t<>::executor_with_default<boost::asio::any_io_executor>;
    using tcp_stream = typename boost::beast::tcp_stream::rebind_executor<executor_with_default>::other;

    boost::beast::ssl_stream<tcp_stream> stream{
            boost::asio::use_awaitable_t<boost::asio::any_io_executor>::as_default_on(
                    boost::beast::tcp_stream(co_await boost::asio::this_coro::executor)),
            ctx_};

    if (!SSL_set_tlsext_host_name(stream.native_handle(), defaultHost))
        throw boost::system::system_error(static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category());

    auto const results = co_await resolver.async_resolve(defaultHost, std::to_string(defaultPort));

    boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(defaultTimeout));

    co_await boost::beast::get_lowest_layer(stream).async_connect(results);

    boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(defaultTimeout));

    co_await stream.async_handshake(boost::asio::ssl::stream_base::client);

    boost::beast::http::request<boost::beast::http::string_body> req{boost::beast::http::verb::post, loginURI,
                                                                     defaultHTTPVersion};
    {
        req.set(boost::beast::http::field::host, defaultHost);
        req.set(boost::beast::http::field::keep_alive, std::string("timeout=") + std::to_string(defaultTimeout));
        req.set(boost::beast::http::field::content_type, "application/json");
        req.set(boost::beast::http::field::accept, "application/json");
        {
            boost::property_tree::ptree json;
            json.put("username", defaultUser);
            json.put("password", defaultPassword);
            std::ostringstream oss;
            boost::property_tree::write_json(oss, json);
            req.body() = oss.str();
            req.prepare_payload();
        }
    }

    boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(defaultTimeout));

    co_await boost::beast::http::async_write(stream, req);

    boost::beast::flat_buffer b;

    boost::beast::http::response<boost::beast::http::dynamic_body> res;

    co_await boost::beast::http::async_read(stream, b, res);

    boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(defaultTimeout));

    auto [ec] = co_await stream.async_shutdown(boost::asio::as_tuple(boost::asio::use_awaitable));
    if (ec) {
        // this is not critical, most probably the connection has been dropped on the server side
        ec = {};
    }

    const auto statusCode = res.result_int();
    const auto body = boost::beast::buffers_to_string(res.body().data());
    const auto json = boost::json::parse(body);
    const auto status = json.get_object().if_contains("status");
    if (status != nullptr && status->is_string()) {
        if (json.at("status").get_string() == "ok") { co_return std::pair(statusCode, json.at("data").get_string()); }
    }

    co_return std::pair(statusCode, body);
}
