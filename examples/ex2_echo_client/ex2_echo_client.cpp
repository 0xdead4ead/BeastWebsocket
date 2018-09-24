#include <iostream>

#include <client.hpp>

using namespace std;

template<typename _CharT, typename _Traits>
auto read_string(basic_istream<_CharT, _Traits>& is, _CharT delim){
    string str;

    std::getline(is, str, delim);
    return str;
}

int main()
{

    ws::client echo;

    echo.on_connect = [](auto & session){
        session.do_handshake("/echo");
    };

    echo.on_handshake = [](auto & /*session*/, auto & res, auto & output, auto & /*next_read*/){
        cout << res << endl;
        cout << "Send: ";
        boost::beast::ostream(output) << read_string(cin, '\n');
    };

    echo.on_message = [](auto & /*session*/, auto & input, auto & output, auto & /*next_read*/){
        cout << "Recv: " << boost::beast::buffers(input.data()) << endl;

        cout << "Send: ";
        boost::beast::ostream(output) << read_string(cin, '\n');
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
