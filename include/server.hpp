#ifndef BEAST_WS_SERVER_HPP
#define BEAST_WS_SERVER_HPP

#include "session.hpp"

namespace ws {

/// \brief ws server class
class server_impl{

    std::function<void(boost::beast::websocket::response_type&)> decorator_;

public:

    std::function<void(session<true>&, boost::beast::multi_buffer&)> on_accept;
    std::function<void(session<true>&, const boost::beast::multi_buffer&, boost::beast::multi_buffer&)> on_message;
    std::function<void(session<true>&, const boost::beast::string_view&)> on_ping;
    std::function<void(session<true>&, const boost::beast::string_view&)> on_pong;
    std::function<void(session<true>&, const boost::beast::string_view&)> on_close;

    explicit server_impl()
    {}

    template<class ResponceDecorator>
    explicit server_impl(ResponceDecorator&& decorator)
        : decorator_{std::forward<ResponceDecorator>(decorator)}
    {}

    template<class ConnectionPtr, class Callback>
    void upgrade_session(const ConnectionPtr& connection, Callback && on_done){
        session<true>::template make<Callback>(connection->release_stream(),
                                                        decorator_,
                                                        on_accept,
                                                        on_message,
                                                        on_ping,
                                                        on_pong,
                                                        on_close,
                                                        std::forward<Callback>(on_done));
    }

}; // server_impl class

using server = server_impl;

} // namespace ws

#endif // BEAST_WS_SERVER_HPP
