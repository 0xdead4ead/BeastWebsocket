#ifndef BEAST_WS_SESSION_HPP
#define BEAST_WS_SESSION_HPP

#include "base.hpp"

namespace ws {

using message_t = boost::beast::string_view;

//###########################################################################

/// \brief session class. Handles an WS server connection
/// \tparam Type of body request message
template<bool isServer, class Body>
class session  : private boost::noncopyable,
        public std::enable_shared_from_this<session<true, Body> >
{

    // Set up after accept handshake
    bool accepted = false;
    // Auto-detection of incoming frame type
    bool auto_frame = true;

    std::function<void(session<true, Body>&)> on_timer_cb;

    const std::function<void(boost::beast::websocket::response_type&)> & decorator_cb_;

    // user handler events
    const std::function<void(session<true, Body>&, boost::beast::multi_buffer&)> & on_accept_cb_;
    const std::function<void(session<true, Body>&, const boost::beast::multi_buffer&, boost::beast::multi_buffer&)> & on_message_cb_;
    const std::function<void(session<true, Body>&, const boost::beast::string_view&)> & on_ping_cb_;
    const std::function<void(session<true, Body>&, const boost::beast::string_view&)> & on_pong_cb_;
    const std::function<void(session<true, Body>&, const boost::beast::string_view&)> & on_close_cb_;

public:

    explicit session(boost::asio::ip::tcp::socket&& socket,
                     const std::function<void(boost::beast::websocket::response_type&)> & decorator_cb,
                     const std::function<void(session<true, Body>&, boost::beast::multi_buffer&)> & on_accept_cb,
                     const std::function<void(session<true, Body>&, const boost::beast::multi_buffer&, boost::beast::multi_buffer&)> & on_message_cb,
                     const std::function<void(session<true, Body>&, const boost::beast::string_view&)> & on_ping_cb,
                     const std::function<void(session<true, Body>&, const boost::beast::string_view&)> & on_pong_cb,
                     const std::function<void(session<true, Body>&, const boost::beast::string_view&)> & on_close_cb)
        : timer_p_{std::make_shared<http::base::timer>(socket.get_executor(),
                                                       (std::chrono::steady_clock::time_point::max)())},
          decorator_cb_{decorator_cb},
          on_accept_cb_{on_accept_cb},
          on_message_cb_{on_message_cb},
          on_ping_cb_{on_ping_cb},
          on_pong_cb_{on_pong_cb},
          on_close_cb_{on_close_cb},
          connection_p_{std::make_shared<base::connection>(std::move(socket))}
    {}

    template<class Callback>
    static void make(boost::asio::ip::tcp::socket&& socket,
                     const std::function<void(boost::beast::websocket::response_type&)> & decorator_cb,
                     const std::function<void(session<true, Body>&, boost::beast::multi_buffer&)> & on_accept_cb,
                     const std::function<void(session<true, Body>&, const boost::beast::multi_buffer&, boost::beast::multi_buffer&)> & on_message_cb,
                     const std::function<void(session<true, Body>&, const boost::beast::string_view&)> & on_ping_cb,
                     const std::function<void(session<true, Body>&, const boost::beast::string_view&)> & on_pong_cb,
                     const std::function<void(session<true, Body>&, const boost::beast::string_view&)> & on_close_cb,
                     Callback&& on_done)
    {
        auto new_session_p = std::make_shared<session<true, Body> >
                (std::move(socket), decorator_cb, on_accept_cb, on_message_cb, on_ping_cb, on_pong_cb, on_close_cb);
        on_done(*new_session_p);
    }

    template<class Request>
    void do_accept(const Request& msg)
    {

        if(accepted)
            return;

        connection_p_->control_callback(
                    std::bind(
                        &session<true, Body>::on_control_callback,
                        this,
                        std::placeholders::_1,
                        std::placeholders::_2));

        timer_p_->stream().expires_after(std::chrono::seconds(10));

        // Accept the websocket handshake
        if(decorator_cb_){
            connection_p_->async_accept_ex(msg, decorator_cb_,
                                           std::bind(
                                               &session<true, Body>::on_accept,
                                               this->shared_from_this(),
                                               std::placeholders::_1));
        }else{
            connection_p_->async_accept(msg,
                                        std::bind(
                                            &session<true, Body>::on_accept,
                                            this->shared_from_this(),
                                            std::placeholders::_1));
        }
    }

    auto & output(){
        return output_buffer_;
    }

    auto & getConnection() const
    {
        return connection_p_;
    }

    void setAutoFrame(){
        auto_frame = true;
    }

    void setTextFrame(){
        auto_frame = false;
        connection_p_->stream().text(true);
    }

    void setBinaryFrame(){
        auto_frame = false;
        connection_p_->stream().binary(true);
    }

    void do_ping(boost::beast::websocket::ping_data const & payload){

        if(!accepted)
            return;

        timer_p_->stream().expires_after(std::chrono::seconds(10));

        connection_p_->async_ping(payload,
                                  std::bind(
                                      &session<true, Body>::on_ping,
                                      this->shared_from_this(),
                                      std::placeholders::_1));
    }

    void do_pong(boost::beast::websocket::ping_data const & payload){

        if(!accepted)
            return;

        timer_p_->stream().expires_after(std::chrono::seconds(10));

        connection_p_->async_pong(payload,
                                  std::bind(
                                      &session<true, Body>::on_pong,
                                      this->shared_from_this(),
                                      std::placeholders::_1));
    }

    void do_close(boost::beast::websocket::close_reason const & reason){

        if(!accepted)
            return;

        timer_p_->stream().expires_after(std::chrono::seconds(10));

        connection_p_->async_close(reason,
                                   std::bind(
                                       &session<true, Body>::on_close,
                                       this->shared_from_this(),
                                       std::placeholders::_1));
    }

    void launch_timer()
    {
        timer_p_->async_wait(
                    std::bind(
                        &session<true, Body>::on_timer,
                        this->shared_from_this(),
                        std::placeholders::_1));
    }

    template<class F>
    void launch_timer(F&& f){

        on_timer_cb = std::forward<F>(f);

        timer_p_->async_wait(
                    std::bind(
                        &session<true, Body>::on_timer,
                        this->shared_from_this(),
                        std::placeholders::_1));
    }

    void do_write(){
        connection_p_->async_write(
            output_buffer_,
                std::bind(
                    &session<true, Body>::on_write,
                    this->shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2));
    }

protected:

    void do_read(){
        timer_p_->stream().expires_after(std::chrono::seconds(10));

        connection_p_->async_read(
                    input_buffer_,
                        std::bind(
                            &session<true, Body>::on_read,
                            this->shared_from_this(),
                            std::placeholders::_1,
                            std::placeholders::_2));
    }

    void on_accept(const boost::system::error_code & ec)
    {
        // Happens when the timer closes the socket
        if(ec == boost::asio::error::operation_aborted)
            return;

        if(ec)
            return http::base::fail(ec, "accept");

        accepted = true;

        if(on_accept_cb_)
            on_accept_cb_(*this, output_buffer_);

        if(output_buffer_.size() > 0)
            do_write();
        else
            do_read();

    }

    void on_control_callback(boost::beast::websocket::frame_type kind,
                             boost::beast::string_view payload){
        if( (kind == boost::beast::websocket::frame_type::ping) && on_ping_cb_)
            on_ping_cb_(*this, payload);
        else if( (kind == boost::beast::websocket::frame_type::pong) && on_pong_cb_)
            on_pong_cb_(*this, payload);
        else if(on_close_cb_)
            on_close_cb_(*this, payload);
    }

    // Called after a ping is sent.
    void on_ping(const boost::system::error_code & ec)
    {
        // Happens when the timer closes the socket
        if(ec == boost::asio::error::operation_aborted)
            return;

        if(ec)
            return http::base::fail(ec, "ping");

        do_read();
    }

    // Called after a pong is sent.
    void on_pong(const boost::system::error_code & ec)
    {
        // Happens when the timer closes the socket
        if(ec == boost::asio::error::operation_aborted)
            return;

        if(ec)
            return http::base::fail(ec, "pong");

        do_read();
    }

    // Called after a close is sent.
    void on_close(const boost::system::error_code & ec)
    {
        // Happens when close times out
        if(ec == boost::asio::error::operation_aborted)
            return;

        if(ec)
            return http::base::fail(ec, "close");

        // At this point the connection is gracefully closed
    }

    void on_timer(boost::system::error_code ec)
    {
        if(ec && ec != boost::asio::error::operation_aborted)
            return http::base::fail(ec, "timer");

        // Verify that the timer really expired since the deadline may have moved.
        if(timer_p_->stream().expiry() <= std::chrono::steady_clock::now())
        {

            if(on_timer_cb)
            {
                on_timer_cb(*this);
                return;
            }

            timer_p_->stream().expires_after(std::chrono::seconds(10));

            connection_p_->async_close(boost::beast::websocket::close_code::normal,
                                       std::bind(
                                           &session<true, Body>::on_close,
                                           this->shared_from_this(),
                                           std::placeholders::_1));
        }

        launch_timer();
    }

    void on_read(const boost::system::error_code & ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // Happens when the timer closes the socket
        if(ec == boost::asio::error::operation_aborted)
            return;

        // This indicates that the websocket_session was closed
        if(ec == boost::beast::websocket::error::closed)
            return;

        if(ec)
            http::base::fail(ec, "read");

        if(on_message_cb_)
            on_message_cb_(*this, input_buffer_, output_buffer_);

        // Clear the buffer
        input_buffer_.consume(input_buffer_.size());

        if(auto_frame)
            //Is this a text frame? If are not, to set binary
            connection_p_->stream().text(connection_p_->stream().got_text());

        if(output_buffer_.size() > 0)
            do_write();
        else
            do_read();

    }


    void on_write(const boost::system::error_code & ec,
                  std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // Happens when the timer closes the socket
        if(ec == boost::asio::error::operation_aborted)
            return;

        if(ec)
            return http::base::fail(ec, "write");

        // Clear the buffer
        output_buffer_.consume(output_buffer_.size());

        // Do another read
        do_read();
    }

    http::base::timer::ptr timer_p_;
    base::connection::ptr connection_p_;

    // io buffers
    boost::beast::multi_buffer input_buffer_;
    boost::beast::multi_buffer output_buffer_;

}; // class session

/// \brief session class. Handles an WS client connection
/// \tparam Type of body response message
template<class Body>
class session<false, Body> : private boost::noncopyable,
        public std::enable_shared_from_this<session<false, Body> >{

    // Auto-detection of incoming frame type
    bool auto_frame = true;

    const std::function<void(boost::beast::websocket::request_type&)> & decorator_cb_;

    // user handler events
    const std::function<void(session<false, Body>&)> & on_connect_cb_;
    const std::function<void(session<false, Body>&, boost::beast::multi_buffer&, bool&)> & on_handshake_cb_;
    const std::function<void(session<false, Body>&, const boost::beast::multi_buffer&, boost::beast::multi_buffer&, bool&)> & on_message_cb_;
    const std::function<void(session<false, Body>&, const boost::beast::string_view&)> & on_ping_cb_;
    const std::function<void(session<false, Body>&, const boost::beast::string_view&)> & on_pong_cb_;
    const std::function<void(session<false, Body>&, const boost::beast::string_view&)> & on_close_cb_;

    bool handshaked = false;

public:

    explicit session(std::string const & host,
                     base::connection::ptr & connection_p,
                     const std::function<void(boost::beast::websocket::request_type&)> & decorator_cb,
                     const std::function<void(session<false, Body>&)> & on_connect_cb,
                     const std::function<void(session<false, Body>&, boost::beast::multi_buffer&, bool&)> & on_handshake_cb,
                     const std::function<void(session<false, Body>&, const boost::beast::multi_buffer&, boost::beast::multi_buffer&, bool&)> & on_message_cb,
                     const std::function<void(session<false, Body>&, const boost::beast::string_view&)> & on_ping_cb,
                     const std::function<void(session<false, Body>&, const boost::beast::string_view&)> & on_pong_cb,
                     const std::function<void(session<false, Body>&, const boost::beast::string_view&)> & on_close_cb)
        : host_{host},
          connection_p_{connection_p},
          decorator_cb_{decorator_cb},
          on_connect_cb_{on_connect_cb},
          on_handshake_cb_{on_handshake_cb},
          on_message_cb_{on_message_cb},
          on_ping_cb_{on_ping_cb},
          on_pong_cb_{on_pong_cb},
          on_close_cb_{on_close_cb}
    {}

    static void on_connect(std::string const & host,
                           base::connection::ptr & connection_p,
                           const std::function<void(boost::beast::websocket::request_type&)> & decorator_cb,
                           const std::function<void(session<false, Body>&)> & on_connect_cb,
                           const std::function<void(session<false, Body>&, boost::beast::multi_buffer&, bool&)> & on_handshake_cb,
                           const std::function<void(session<false, Body>&, const boost::beast::multi_buffer&, boost::beast::multi_buffer&, bool&)> & on_message_cb,
                           const std::function<void(session<false, Body>&, const boost::beast::string_view&)> & on_ping_cb,
                           const std::function<void(session<false, Body>&, const boost::beast::string_view&)> & on_pong_cb,
                           const std::function<void(session<false, Body>&, const boost::beast::string_view&)> & on_close_cb)
    {
        auto new_session_p = std::make_shared<session<false, Body>>
                (host, connection_p, decorator_cb, on_connect_cb, on_handshake_cb, on_message_cb, on_ping_cb, on_pong_cb, on_close_cb);
        if(on_connect_cb)
            on_connect_cb(*new_session_p);
    }

    void do_handshake(boost::beast::string_view target){

        if(handshaked)
            return;

        connection_p_->control_callback(
                    std::bind(
                        &session<false, Body>::on_control_callback,
                        this,
                        std::placeholders::_1,
                        std::placeholders::_2));

        // Perform the websocket handshake
        if(decorator_cb_)
            connection_p_->async_handshake_ex(host_, target,
                                              decorator_cb_,
                                              std::bind(
                                                  &session<false, Body>::on_handshake,
                                                  this->shared_from_this(),
                                                  std::placeholders::_1));
        else
            connection_p_->async_handshake(host_, target,
                                           std::bind(
                                               &session<false, Body>::on_handshake,
                                               this->shared_from_this(),
                                               std::placeholders::_1));
    }

    auto & output(){
        return output_buffer_;
    }

    auto & getConnection() const
    {
        return connection_p_;
    }

    void setAutoFrame(){
        auto_frame = true;
    }

    void setTextFrame(){
        auto_frame = false;
        connection_p_->stream().text(true);
    }

    void setBinaryFrame(){
        auto_frame = false;
        connection_p_->stream().binary(true);
    }

    void do_ping(boost::beast::websocket::ping_data const & payload){

        if(!handshaked)
            return;

        connection_p_->async_ping(payload,
                                  std::bind(
                                      &session<false, Body>::on_ping,
                                      this->shared_from_this(),
                                      std::placeholders::_1));
    }

    void do_pong(boost::beast::websocket::ping_data const & payload){

        if(!handshaked)
            return;

        connection_p_->async_pong(payload,
                                  std::bind(
                                      &session<false, Body>::on_pong,
                                      this->shared_from_this(),
                                      std::placeholders::_1));
    }

    void do_close(boost::beast::websocket::close_reason const & reason){

        if(!handshaked)
            return;

        connection_p_->async_close(reason,
                                   std::bind(
                                       &session<false, Body>::on_close,
                                       this->shared_from_this(),
                                       std::placeholders::_1));
    }

protected:

    void do_read(){
        connection_p_->async_read(input_buffer_,
                                  std::bind(
                                      &session<false, Body>::on_read,
                                      this->shared_from_this(),
                                      std::placeholders::_1,
                                      std::placeholders::_2));
    }

    void do_write(bool next_read){
        connection_p_->async_write(output_buffer_,
                                   std::bind(
                                       &session<false, Body>::on_write,
                                       this->shared_from_this(),
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       next_read));
    }

    void on_handshake(const boost::system::error_code & ec)
    {
        if(ec)
            return http::base::fail(ec, "handshake");

        handshaked = true;

        bool next_read = true;

        if(on_handshake_cb_)
            on_handshake_cb_(*this, output_buffer_, next_read);

        if(output_buffer_.size() > 0)
            do_write(next_read);
    }

    void on_control_callback(boost::beast::websocket::frame_type kind,
                             boost::beast::string_view payload){
        if( (kind == boost::beast::websocket::frame_type::ping) && on_ping_cb_)
            on_ping_cb_(*this, payload);
        else if( (kind == boost::beast::websocket::frame_type::pong) && on_pong_cb_)
            on_pong_cb_(*this, payload);
        else if(on_close_cb_)
            on_close_cb_(*this, payload);
    }

    // Called after a ping is sent.
    void on_ping(const boost::system::error_code & ec)
    {
        // Happens when the timer closes the socket
        if(ec == boost::asio::error::operation_aborted)
            return;

        if(ec)
            return http::base::fail(ec, "ping");

        do_read();
    }

    // Called after a pong is sent.
    void on_pong(const boost::system::error_code & ec)
    {
        // Happens when the timer closes the socket
        if(ec == boost::asio::error::operation_aborted)
            return;

        if(ec)
            return http::base::fail(ec, "pong");

        do_read();
    }

    void on_close(const boost::system::error_code & ec)
    {
        if(ec)
            return http::base::fail(ec, "close");
    }

    void on_write(const boost::system::error_code & ec,
                  std::size_t bytes_transferred, bool next_read)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return http::base::fail(ec, "write");

        // Clear the buffer
        output_buffer_.consume(output_buffer_.size());

        // Read a message into our buffer
        if(next_read)
            do_read();

    }

    void on_read(const boost::system::error_code & ec,
                 std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return http::base::fail(ec, "read");

        bool next_read = true;

        if(on_message_cb_)
            on_message_cb_(*this, input_buffer_, output_buffer_, next_read);

        // Clear the buffer
        input_buffer_.consume(input_buffer_.size());

        if(auto_frame)
            //Is this a text frame? If are not, to set binary
            connection_p_->stream().text(connection_p_->stream().got_text());

        if(output_buffer_.size() > 0)
            do_write(next_read);
    }

    std::string host_;
    base::connection::ptr & connection_p_;

    // io buffers
    boost::beast::multi_buffer input_buffer_;
    boost::beast::multi_buffer output_buffer_;

}; // class session


} // namespace ws

#endif // BEAST_WS_SESSION_HPP
