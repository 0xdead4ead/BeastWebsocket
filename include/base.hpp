#ifndef BEAST_WS_BASE_HPP
#define BEAST_WS_BASE_HPP

#include <beast_http_server/include/base.hpp>

#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>


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

public:

    explicit connection_base(boost::asio::io_context::executor_type executor)
        : strand_{executor}
    {}

    template<class F>
    void async_handshake(boost::beast::string_view host,
                         boost::beast::string_view target,
                         F&& f){
        derived().stream().async_handshake(host, target,
                                           std::forward<F>(f));
    }

    template<class F, class D>
    void async_handshake_ex(boost::beast::string_view host,
                            boost::beast::string_view target,
                            const D& d,
                            F&& f){
        derived().stream().async_handshake_ex(host, target,
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
    void async_close(boost::beast::websocket::reason_string const & reason, F&& f){
        derived().stream().async_close(reason,
                                       boost::asio::bind_executor(
                                           strand_, std::forward<F>(f)));
    }

    template<class F>
    void async_close(boost::beast::websocket::close_code const & code, F&& f){
        derived().stream().async_close(code,
                                       boost::asio::bind_executor(
                                           strand_, std::forward<F>(f)));
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
        : base_t{socket.get_executor()},
          ws_{std::move(socket)}
    {}

    // Constructor for client to server connection
    template<class F>
    explicit connection(
            boost::asio::io_service& ios,
            const boost::asio::ip::tcp::endpoint& endpoint, F&& f)
        : base_t{ios.get_executor()},
          ws_{ios}
    {
        ws_.next_layer().async_connect(endpoint, std::forward<F>(f));
    }

    auto & stream(){
        return ws_;
    }

}; // plain_connection class

} // namespace base

} // namespace ws

#endif // BEAST_WS_BASE_HPP
