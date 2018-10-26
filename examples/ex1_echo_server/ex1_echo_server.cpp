#include <iostream>

#include <server.hpp>
#include <BeastHttp/include/server.hpp>

using namespace std;

template<class Request>
auto make_response(const Request & req, const string & user_body){

    boost::beast::http::string_body::value_type body(user_body);

    auto const body_size = body.size();

    boost::beast::http::response<boost::beast::http::string_body> res{
         std::piecewise_construct,
         std::make_tuple(std::move(body)),
         std::make_tuple(boost::beast::http::status::ok, req.version())};

    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, "text/html");
    res.content_length(body_size);
    res.keep_alive(req.keep_alive());

    return res;

}

int main()
{

    http::server my_http_server;
    ws::server echo;

    echo.on_accept = [](auto & /*session*/, auto & /*output*/){
        // if it this pull server, an output buffer must be blank
    };

    echo.on_message = [](auto & /*session*/, auto & input, auto & output){
        cout << boost::beast::buffers(input.data()) << endl;
        boost::beast::ostream(output) << boost::beast::buffers(input.data()); // echo
    };

    my_http_server.get("/echo", [&echo](auto & req, auto & session){
        cout << req << endl;
        // See if it is a WebSocket Upgrade
        if(boost::beast::websocket::is_upgrade(req))
        {
            echo.upgrade_session(session.getConnection(), [req](auto & session){
                session.do_accept(req);
            });
        }

    });

    my_http_server.all(".*", [](auto & req, auto & session){
        cout << req << endl; // any
        session.do_write(make_response(req, "error\n"));
    });

    my_http_server.listen("127.0.0.1", 80, [](auto & session){
        http::base::out(session.getConnection()->stream().remote_endpoint().address().to_string() + " connected");
        session.do_read();
    });

    http::base::processor::get().register_signals_handler([](int signal){
        if(signal == SIGINT)
            http::base::out("Interactive attention signal");
        else if(signal == SIGTERM)
            http::base::out("Termination request");
        else
            http::base::out("Quit");
        http::base::processor::get().stop();
    }, std::vector<int>{SIGINT,SIGTERM, SIGQUIT});

    uint32_t pool_size = boost::thread::hardware_concurrency();
    http::base::processor::get().start(pool_size == 0 ? 4 : pool_size << 1);
    http::base::processor::get().wait();

    return 0;
}
