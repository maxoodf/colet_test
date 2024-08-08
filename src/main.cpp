#include "HTTPClient.h"
#include "WebsocketClient.h"

#include <boost/asio/co_spawn.hpp>

#include <iostream>

int main() {
    try {
        std::string token;
        {
            boost::asio::io_context ioc;
            boost::asio::ssl::context ctx{boost::asio::ssl::context::tlsv12_client};
            std::pair<uint, std::string> result;
            {
                HTTPClient httpClient(ctx);
                boost::asio::co_spawn(ioc, httpClient.getSessionToken(),
                                      [&result](const std::exception_ptr &e, std::pair<uint, std::string> ret) {
                                          if (e) { std::rethrow_exception(e); }
                                          result = std::move(ret);
                                      });
            }

            ioc.run();

            if (result.first != 200) {
                std::cerr << "Login request is failed with the status code " << result.first
                          << " and message: " << result.second << '\n';

                return EXIT_FAILURE;
            }

            token = std::move(result.second);
        }

        std::set<std::string> competitionNames;
        {
            boost::asio::io_context ioc;
            boost::asio::ssl::context ctx{boost::asio::ssl::context::tlsv12_client};
            {
                WebsocketClient websocketClient(ctx);
                boost::asio::co_spawn(ioc, websocketClient.getCompetitionName(token),
                                      [&competitionNames](const std::exception_ptr &e, std::set<std::string> ret) {
                                          if (e) { std::rethrow_exception(e); }
                                          competitionNames = std::move(ret);
                                      });
                ioc.run();
            }
        }

        for (const auto &i: competitionNames) { std::cout << i << '\n'; }

        return EXIT_SUCCESS;
    } catch (std::exception &ex) { std::cerr << "Error: " << ex.what() << '\n'; }

    return EXIT_FAILURE;
}
