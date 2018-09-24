#ifndef BEAST_WS_CLIENT_HPP
#define BEAST_WS_CLIENT_HPP

#include "base.hpp"
#include "session.hpp"

namespace ws{

/// \brief Class for communication with a remote host
/// \tparam Body type for response message
template<class ResBody>
class client_impl{

    template<class Callback0>
    bool process(std::string const & host, uint32_t port, Callback0 && on_error_handler){
        connection_p_ = http::base::processor::get()
                .create_connection<base::connection>(host,
                                                     port,
                                                     [this, host, on_error = std::forward<Callback0>(on_error_handler)](const boost::system::error_code & ec){
            if(ec){
                http::base::fail(ec, "connect");
                on_error(ec);
                return;
            }

            session<false, ResBody>::on_connect(connection_p_, decorator_, on_connect, on_handshake, on_message, on_ping, on_pong, on_close);
        });

        if(!connection_p_)
            return false;

        return true;
    }

    std::function<void(boost::beast::websocket::request_type&)> decorator_;
    base::connection::ptr connection_p_;

public:

    std::function<void(session<false, ResBody>&)> on_connect;
    std::function<void(session<false, ResBody>&, const boost::beast::websocket::response_type&, boost::beast::multi_buffer&, bool&)> on_handshake;
    std::function<void(session<false, ResBody>&, const boost::beast::multi_buffer&, boost::beast::multi_buffer&, bool&)> on_message;
    std::function<void(session<false, ResBody>&, const boost::beast::string_view&)> on_ping;
    std::function<void(session<false, ResBody>&, const boost::beast::string_view&)> on_pong;
    std::function<void(session<false, ResBody>&, const boost::beast::string_view&)> on_close;

    explicit client_impl()
        : connection_p_{nullptr}
    {}

    template<class RequestDecorator>
    explicit client_impl(RequestDecorator && decorator)
        : decorator_{std::forward<RequestDecorator>(decorator)},
          connection_p_{nullptr}
    {}

    template<class Callback0>
    bool invoke(std::string const & host, uint32_t port, Callback0 && on_error_handler){
        return process(host, port, std::forward<Callback0>(on_error_handler));
    }

}; // client_impl class

using client = client_impl<boost::beast::http::string_body>;

} // namespace ws

#endif // BEAST_WS_CLIENT_HPP
