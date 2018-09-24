#ifndef BEAST_WS_BASE_HPP
#define BEAST_WS_BASE_HPP

#include <beast_http_server/include/base.hpp>

#include <boost/beast/websocket.hpp>


#if BEAST_HTTP_SERVER_VERSION < 102
#error "BEAST_HTTP_SERVER_VERSION must be >= 102"
#endif

namespace ws {

namespace base {

/// \brief The connection class
template<class Derived>
class connection_base : private boost::noncopyable {

    Derived& derived()
    {
        return static_cast<Derived&>(*this);
    }

protected:

    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    /*boost::beast::string_view*/ std::string host_; // for handshake operation

public:

    explicit connection_base(boost::asio::io_context::executor_type executor, const boost::beast::string_view & host)
        : strand_{executor}, host_{host}
    {}

    template<class F>
    void async_handshake(boost::beast::string_view target,
                         F&& f){
        derived().stream().async_handshake(host_, target,
                                           std::forward<F>(f));
    }

    template<class F>
    void async_handshake(boost::beast::websocket::response_type& res,
                         boost::beast::string_view target,
                         F&& f){
        derived().stream().async_handshake(res, host_, target,
                                           std::forward<F>(f));
    }

    template<class F, class D>
    void async_handshake_ex(boost::beast::string_view target,
                            const D& d,
                            F&& f){
        derived().stream().async_handshake_ex(host_, target,
                                              d, std::forward<F>(f));
    }

    template<class F, class D>
    void async_handshake_ex(boost::beast::websocket::response_type& res,
                            boost::beast::string_view target,
                            const D& d,
                            F&& f){
        derived().stream().async_handshake_ex(res, host_, target,
                                              d, std::forward<F>(f));
    }

    template<class R, class F>
    void async_accept(const R& r, F&& f){
        derived().stream().async_accept(
                    r, boost::asio::bind_executor(
                        strand_, std::forward<F>(f)));
    }

    template<class R, class F, class D>
    void async_accept_ex(const R& r, const D& d, F&& f){
        derived().stream().async_accept_ex(
                    r, d, boost::asio::bind_executor(
                        strand_, std::forward<F>(f)));
    }


    template <class F, class B>
    void async_write(const B& buf, F&& f){
        derived().stream().async_write(
                    buf.data(),
                    boost::asio::bind_executor(
                        strand_, std::forward<F>(f)));
    }

    template <class F, class B>
    void async_read(B& buf, F&& f){
        derived().stream().async_read(
                    buf,
                    boost::asio::bind_executor(
                        strand_, std::forward<F>(f)));
    }

    template<class F>
    void async_ping(boost::beast::websocket::ping_data const & payload, F&& f){
        derived().stream().async_ping(payload,
                                      boost::asio::bind_executor(
                                          strand_, std::forward<F>(f)));
    }

    template<class F>
    void async_pong(boost::beast::websocket::ping_data const& payload, F&& f){
        derived().stream().async_pong(payload,
                                      boost::asio::bind_executor(
                                          strand_, std::forward<F>(f)));
    }

    template<class F>
    void async_close(boost::beast::websocket::close_reason const & reason, F&& f){
        derived().stream().async_close(reason,
                                       boost::asio::bind_executor(
                                           strand_, std::forward<F>(f)));
    }

    template<class R>
    auto accept(const R& r){
        boost::beast::error_code ec;

        derived().stream().accept(r, ec);

        if(ec)
            http::base::fail(ec, "accept");

        return ec;
    }

    template<class R, class D>
    auto accept_ex(const R& r, const D& d){
        boost::beast::error_code ec;

        derived().stream().accept_ex(r, d, ec);

        if(ec)
            http::base::fail(ec, "accept");

        return ec;
    }

    auto handshake(boost::beast::string_view target){
        boost::beast::error_code ec;

        derived().stream().handshake(host_, target, ec);

        if(ec)
            http::base::fail(ec, "handshake");

        return ec;
    }

    auto handshake(boost::beast::websocket::response_type& res,
                   boost::beast::string_view target){
        boost::beast::error_code ec;

        derived().stream().handshake(res, host_, target, ec);

        if(ec)
            http::base::fail(ec, "handshake");

        return ec;
    }

    template<class D>
    auto handshake_ex(boost::beast::string_view target,
                      const D& d){
        boost::beast::error_code ec;

        derived().stream().handshake_ex(host_, target, d, ec);

        if(ec)
            http::base::fail(ec, "handshake");

        return ec;
    }

    template<class D>
    auto handshake_ex(boost::beast::websocket::response_type& res,
                      boost::beast::string_view target,
                      const D& d){
        boost::beast::error_code ec;

        derived().stream().handshake_ex(res, host_, target, d, ec);

        if(ec)
            http::base::fail(ec, "handshake");

        return ec;
    }

    template<class B>
    auto write(const B& buf){
        boost::beast::error_code ec;

        derived().stream().write(buf.data(), ec);

        if(ec)
            http::base::fail(ec, "write");

        return ec;
    }

    template<class B>
    auto read(B& buf){
        boost::beast::error_code ec;

        derived().stream().read(buf, ec);

        if(ec)
            http::base::fail(ec, "read");

        return ec;
    }

    auto ping(boost::beast::websocket::ping_data const & payload){
        boost::beast::error_code ec;

        derived().stream().ping(payload, ec);

        if(ec)
            http::base::fail(ec, "ping");

        return ec;
    }

    auto pong(boost::beast::websocket::ping_data const & payload){
        boost::beast::error_code ec;

        derived().stream().pong(payload, ec);

        if(ec)
            http::base::fail(ec, "pong");

        return ec;
    }

    auto close(boost::beast::websocket::close_reason const & reason){
        boost::beast::error_code ec;

        derived().stream().close(reason);

        if(ec)
            http::base::fail(ec, "close");

        return ec;
    }

    template<class F>
    void control_callback(F&& f){
        derived().stream().control_callback(std::forward<F>(f));
    }

}; // connection class

/// \brief The plain connection class
class connection : public connection_base<connection> {

    using base_t = connection_base<connection>;

    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;

public:

    using ptr = std::shared_ptr<connection>;

    // Constructor for server to client connection
    explicit connection(boost::asio::ip::tcp::socket&& socket)
        : base_t{socket.get_executor(), {}},
          ws_{std::move(socket)}
    {}

    // Constructor for client to server connection
    template<class F>
    explicit connection(
            boost::asio::io_service& ios,
            const boost::asio::ip::tcp::endpoint& endpoint, F&& f)
        : base_t{ios.get_executor(), endpoint.address().to_string()},
          ws_{ios}
    {
        ws_.next_layer().async_connect(endpoint, std::forward<F>(f));
    }

    explicit connection(
            boost::asio::io_service& ios,
            const boost::asio::ip::tcp::endpoint& endpoint)
        : base_t{ios.get_executor(), endpoint.address().to_string()},
          ws_{ios}
    {
        boost::beast::error_code ec;
        ws_.next_layer().connect(endpoint, ec);

        if(ec)
            http::base::fail(ec, "connect");
    }

    auto & stream(){
        return ws_;
    }

}; // plain_connection class

} // namespace base

template<class Body>
auto accept(const base::connection::ptr & connection_p, const boost::beast::http::request<Body> & msg){
    return connection_p->accept(msg);
}

template<class Body, class ResponseDecorator>
auto accept_ex(const base::connection::ptr & connection_p,
               const boost::beast::http::request<Body> & msg, const ResponseDecorator & decorator){
    return connection_p->accept_ex(msg, decorator);
}

auto handshake(const base::connection::ptr & connection_p,
               boost::beast::string_view target){
    return connection_p->handshake(target);
}

auto handshake(const base::connection::ptr & connection_p, boost::beast::websocket::response_type& res,
               boost::beast::string_view target){
    return connection_p->handshake(res, target);
}

template<class RequestDecorator>
auto handshake_ex(const base::connection::ptr & connection_p,
                  boost::beast::string_view target, const RequestDecorator & decorator){
    return connection_p->handshake(target, decorator);
}

template<class RequestDecorator>
auto handshake_ex(const base::connection::ptr & connection_p,
                  boost::beast::websocket::response_type& res,
                  boost::beast::string_view target, const RequestDecorator & decorator){
    return connection_p->handshake(res, target, decorator);
}

template<class StreamBuffer>
auto send(const base::connection::ptr & connection_p, const StreamBuffer& buffer){
    return connection_p->write(buffer);
}

template<class StreamBuffer>
auto recv(const base::connection::ptr & connection_p, StreamBuffer& buffer){
    return connection_p->read(buffer);
}

} // namespace ws

#endif // BEAST_WS_BASE_HPP
