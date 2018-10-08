#include <iostream>

#include <server.hpp>
#include <beast_http_server/include/server.hpp>

//for storing messages and client sessions
#include <unordered_map>
#include <vector>
#include <mutex>

#include "../chat_message.hpp"

template<class Request>
auto make_response(const Request & req, const std::string & user_body){

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

using wss = ws::server_impl<boost::beast::http::string_body>;
using client_session_ptr = std::shared_ptr<ws::session<true>>;

// client struct
struct session_box{
    client_session_ptr session_p;
    std::string nickname;
};

// client session storage
static std::unordered_map<ws::session<true>*, session_box> clients;

// message storage (chat room)
static std::vector<chat::Message> messages;

static std::mutex main_mutex;

auto address_string(const ws::base::connection::ptr & connection){
    return connection->stream().next_layer().remote_endpoint().address().to_string();
}

int main()
{

    http::server instance;
    wss chat{[](auto & res){
            res.insert(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        }};

    chat.on_accept = [](auto & /*session*/, auto & output){
        // Hello msg from server (push request)
        boost::beast::ostream(output) << "What is your name?";
    };

    chat.on_message = [](auto & session, auto & input, auto & output){

        auto input_message = boost::beast::buffers_to_string(input.data());

        // Received answer on Hello msg
        if(input_message.substr(0,11) == "My name is "){
            std::lock_guard<std::mutex> lock_{main_mutex};

            // Send hello
            boost::beast::ostream(output) << std::string("Hello ") + input_message.substr(11);

            auto new_client_ = session_box{session.shared_from_this(), input_message.substr(11)};

            messages.push_back({new_client_.nickname, "Input to chat room!"});

            // Serializing and send last messages to remote host
            std::string output_string;
            chat::Serializer<chat::Inv, chat::Message> s{output_string};
            s.advance(messages);
            boost::beast::ostream(output) << output_string;

            // Serializing and broadcasting last message
            for(auto const & client : clients){
                std::string output_string;
                chat::Serializer<chat::Inv, chat::Message> s{output_string};
                s.advance({messages.back()});
                boost::beast::ostream(client.second.session_p->output()) << output_string;
                client.second.session_p->do_write();
            }

            // push new client to the list
            clients.insert({&session, new_client_});

            session.launch_timer([](auto & session){

                session.do_ping({}); // after expires 10 seconds, if not answers send ping message

                session.launch_timer([](auto & session){
                    // remote host is not answer. close...
                    session.do_close(boost::beast::websocket::close_code::normal);
                });
            });

            return;
        }

        // Reseived new message from the client

        std::lock_guard<std::mutex> lock_{main_mutex};

        // Parsing
        std::vector<chat::Message> new_messages;
        chat::Parser<chat::Inv, chat::Message> p{new_messages};
        boost::system::error_code ec;
        p.advance(input_message, ec);

        if(ec){
            http::base::out("Received invalid chat message. Ignored");
            // Output buffer is empty
            session.do_read();
        }
        else{
            // Messages is valid!
            messages.insert(messages.cend(), new_messages.begin(), new_messages.end()); // add msg to list

            // The user must see his message!
            boost::beast::ostream(output) << input_message;

            for(auto const & client : clients)
                if((session.getConnection() != client.second.session_p->getConnection())
                        && client.second.session_p->getConnection()->stream().next_layer().is_open()){
                    boost::beast::ostream(client.second.session_p->output()) << input_message; // Broadcasting received messages
                    client.second.session_p->do_write();
                }
        }

        session.launch_timer([](auto & session){

            session.do_ping({}); // after expires 10 seconds, if not answers send ping message

            session.launch_timer([](auto & session){
                // remote host is not answer. close...
                session.do_close(boost::beast::websocket::close_code::normal);
            });
        });
    };

    chat.on_ping = [](auto & /*session*/, auto &/* payload*/){
        // client send 'ping'
        // pong response automatically sending
    };

    chat.on_pong = [](auto & session, auto &/* payload*/){
        // client send 'pong'
        // client is online
        // reset timer
        session.launch_timer([](auto & session){

            session.do_ping({}); // after expires 10 seconds, if not answers send ping message

            session.launch_timer([](auto & session){
                // remote host is not answer. close...
                session.do_close(boost::beast::websocket::close_code::normal);
            });
        });
    };

    chat.on_close = [](auto & session, auto &/* payload*/){
        // client close connection
        std::lock_guard<std::mutex> lock_{main_mutex};

        if(session.getConnection()->stream().next_layer().is_open()){
            messages.push_back({"is leaving", clients.at(&session).nickname});

            for(auto const & client : clients)
                if((session.getConnection() != client.second.session_p->getConnection())
                        && client.second.session_p->getConnection()->stream().next_layer().is_open()){
                    std::string output_string;
                    chat::Serializer<chat::Inv, chat::Message> s{output_string};
                    s.advance({messages.back()});
                    boost::beast::ostream(client.second.session_p->output()) << output_string; // Broadcasting last message
                    client.second.session_p->do_write();
                }
        }
    };

    instance.get("/ws", [&chat](auto & req, auto & session){
        //std::cout << req << std::endl;
        // See if it is a WebSocket Upgrade
        if(boost::beast::websocket::is_upgrade(req))
        {
            if(!req.count(boost::beast::http::field::sec_websocket_protocol))
                return session.do_write(make_response(req, "Error! Missing subprotocol\n"));

            if(req.at(boost::beast::http::field::sec_websocket_protocol) != "chat")
                return session.do_write(make_response(req, "Error! Invalid subprotocol\n"));

            chat.upgrade_session(session.getConnection(), [req](auto & session){
                session.do_accept(req);
            });
        }
    });

    instance.all(".*", [](auto & req, auto & session){
        std::cout << req << std::endl; // any
        session.do_write(make_response(req, "Error\n"));
    });

    instance.listen("127.0.0.1", 80, [](auto & session){
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
