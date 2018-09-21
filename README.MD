# SYNOPSIS

Easy WebSocket server and client library implemented using C++14 and Boost.Beast. To start two-way communication, you need an initial handshake (rfc6455 section 1.3). The project uses [`beast_http_server`](https://github.com/0xdead4ead/beast_http_server) with all its dependencies.

# FEATURES

* Header-only
* Supported text/binary data frames, control ping-pong, close frames.
* Asynchronous/ request, response handling
* Thread pool support
* Timer manage (default timeout: 10 seconds, default action: Closing connection)
* Platform independent

# AT SOON...

* Synchronous handling
* TLS/SSL

# USAGE

Create HTTP server and WS instance:

```cpp

    http::server my_http_server;
    ws::server echo;

```

Define handlers to accept connection and received message:

```cpp

    echo.on_accept = [](auto & session, auto & output){
        // if it this pull server, an output buffer must be blank
    };

    echo.on_message = [](auto & session, auto & input, auto & output){
        cout << boost::beast::buffers(input.data()) << endl;
        boost::beast::ostream(output) << boost::beast::buffers(input.data()); // echo
    };

```

Start listening server on localhost:80 with new route for GET request with "/echo" resource:

```cpp

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

    my_http_server.listen("127.0.0.1", 80, [](auto & session){
        http::base::out("New client!!!");
        session.do_read();
    });
    
```

Run the I/O service on the requested number of threads:

```cpp

    uint32_t pool_size = boost::thread::hardware_concurrency() * 2;
    http::base::processor::get().start( pool_size > 0 ? pool_size :  4 );
    http::base::processor::get().wait();

```

Define handlers, connect to remote host:

```cpp

    ws::client echo;

    echo.on_connect = [](auto & session){
        session.do_handshake("/echo");
    };

    echo.on_handshake = [](auto &, auto & output, auto &){
        boost::beast::ostream(output) << "hello!";
    };

    echo.on_message = [](auto & session, auto & input, auto &, auto &){
        cout << boost::beast::buffers(input.data()) << endl;
        // The answer is received, closing stream and stop procs!
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

```

Copyright © 2018 0xdead4ead
