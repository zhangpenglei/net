//
// Created by lg on 18-3-13.
//
#pragma once

#include "EventLoop.h"
#include "Connector.h"
#include "Any.h"

namespace net{
    class TcpClient {
        TcpClient(EventLoop* loop,
                  const InetAddress& serverAddr,
                  const std::string& nameArg);
        ~TcpClient();

        void connect();
        void disconnect();
        void stop();

        void enable_retry(){_retry=true;}
        void retry();
        const std::string& get_name(){return _name;}

        void set_context(const Any&a){
            _context=a;
        }

        void set_context(Any&&a){
            _context=std::move(a);
        }

        Any&get_context(){
            return _context;
        }

    private:
        void new_connection(int fd,InetAddress addr);
        enum Status {
            Disconnected = 0,
            Connecting = 1,
             Connected = 2,
            Disconnecting = 3,
        };
    private:
        EventLoop*_loop;
        std::string _name;
        InetAddress _peer_addr;
        Connector _connector;
        std::atomic<bool>  _retry;
        std::atomic<Status>  _status;
        TCPConnPtr _connection;

        Any _context;
    };

}



