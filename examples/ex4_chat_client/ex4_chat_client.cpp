#include <iostream>

#include <client.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "../chat_message.hpp"

using namespace std;

template<typename _CharT, typename _Traits>
auto read_string(basic_istream<_CharT, _Traits>& is, _CharT delim){
    string str;

    std::getline(is, str, delim);
    return str;
}

static std::string my_name;
static std::shared_ptr<ws::session<false>> my_session_p;

static std::mutex main_mutex;
static std::condition_variable main_cond;

int main()
{

    // Launch thread for input console
    std::thread input_thread{[](){

            std::string input_string;
            std::string output_string;

            std::unique_lock<std::mutex> lock{main_mutex};
            if(!my_session_p)
                main_cond.wait(lock);

            for(;;){

                input_string = read_string(cin, '\n');
                cout << endl;

                chat::Message m{input_string, my_name};
                chat::Serializer<chat::Inv, chat::Message> s{output_string};
                s.advance({m});
                boost::beast::ostream(my_session_p->output()) << output_string;
                my_session_p->do_write();

                input_string.clear();
                output_string.clear();
            }
    }};

    ws::client echo{[](auto & req){
            req.insert(boost::beast::http::field::sec_websocket_protocol, "chat");
        }};

    echo.on_connect = [](auto & session){
        session.do_handshake("/ws");
    };

    echo.on_handshake = [](auto & session, auto & /*res*/, auto & /*output*/, auto & /*next_read*/){
        cout << "Successful handshake!" << endl;
        session.do_read();
        //Waiting for a request from the server
    };

    echo.on_message = [](auto & session, auto & input, auto & output, auto & /*next_read*/){
        if(boost::beast::buffers_to_string(input.data()) == "What is your name?") // req msg from the server
        {
            cout << "Enter your name... : ";
            my_name = read_string(cin, '\n');
            boost::beast::ostream(output) << "My name is " << my_name;
            return;
        }

        string input_string;

        if(boost::beast::buffers_to_string(input.data()).substr(0, my_name.size() + 6) == string("Hello ") + my_name){ // hello msg from the server
            input_string = boost::beast::buffers_to_string(input.data()).substr(my_name.size() + 6);

            my_session_p = session.shared_from_this();
            main_cond.notify_one();
        }
        else
            input_string = boost::beast::buffers_to_string(input.data());

        auto input_messages = std::vector<chat::Message>{};

        chat::Parser<chat::Inv, chat::Message> p{input_messages};
        boost::system::error_code ec;
        p.advance(input_string, ec);

        if(ec)
            http::base::out("Received invalid chat message. Ignored");
        else
            for(auto const & m : input_messages)
                cout << m.nickname_ << " : " << m.payload_ << endl;

        session.do_read();
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
