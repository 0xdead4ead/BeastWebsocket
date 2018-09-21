#include <iostream>

#include <client.hpp>

using namespace std;


int main()
{
    ws::client echo;

    echo.on_connect = [](auto & session){
        session.do_handshake("/echo");
    };

    echo.on_handshake = [](auto &, auto & output, auto &){
        boost::beast::ostream(output) << "hello!";
    };

    echo.on_message = [](auto & session, auto & input, auto &, auto &){
        cout << boost::beast::buffers(input.data()) << endl;
        session.do_close(boost::beast::websocket::close_code::normal);
        http::base::processor::get().stop();
    };

    if(!echo.invoke("127.0.0.1", 80, [](auto & error){
        cout << "Connection failed with code " << error << endl;
        http::base::processor::get().stop();
    })){
        cout << "Failed to resolve address!" << endl;
        return -1;
    }

    uint32_t pool_size = boost::thread::hardware_concurrency();
    http::base::processor::get().start(pool_size == 0 ? 4 : pool_size << 1);
    http::base::processor::get().wait();

    return 0;
}
