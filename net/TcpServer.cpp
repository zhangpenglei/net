//
// Created by lg on 18-3-13.
//

#include "TcpServer.h"
#include"Acceptor.h"
#include "Log.h"
#include"TcpConnection.h"

namespace net {
    using std::placeholders::_1;
    using std::placeholders::_2;

    TcpServer::TcpServer(EventLoop *loop, const InetAddress &addr, const std::string &name, size_t threadSize)
            : _pool(threadSize)
            , _loop(loop)
            ,_accepter(loop,addr){
        LOG_TRACE<<"server";
        _accepter.set_new_connection_cb(std::bind(&TcpServer::handle_new_connection, this, _1, _2));
    }

    TcpServer::~TcpServer() {

    }

    void TcpServer::run() {
        _pool.run();
        _accepter.listen();
    }

    void TcpServer::stop() {
       _loop->run_in_loop(std::bind(&TcpServer::stop_in_loop,this));
        _pool.join();
    }

    void TcpServer::stop_in_loop(){
        _accepter.stop();

        for(auto&conn:_connections){
            conn.second->get_loop()->run_in_loop(std::bind(&TcpConnection::close,conn.second));
        }
        _pool.stop();
    }

    void TcpServer::handle_new_connection(int fd, const InetAddress &addr) {
        LOG_TRACE << "get_fd=" << fd;


        EventLoop* loop = _pool.get_next_loop();


        TCPConnPtr conn(new TcpConnection(_next_conn_id++,loop, fd,_addr,addr));

        conn->set_message_cb(_message_cb);
        conn->set_connection_cb(_connecting_cb);
        conn->set_close_cb(std::bind(&TcpServer::remove_connection, this, _1));

        loop->run_in_loop(std::bind(&TcpConnection::attach_to_loop, conn));
        _connections[conn->get_id()] = conn;
    }

    void TcpServer::remove_connection(const TCPConnPtr& conn) {

        _loop->run_in_loop(std::bind(&TcpServer::remove_connection_in_loop, this, conn));
    }

    void TcpServer::remove_connection_in_loop(const TCPConnPtr& conn) {
        _connections.erase(conn->get_id());
        //EventLoop* loop = conn->get_loop();
        //loop->queue_in_loop(std::bind(&TcpConnection::connectDestroyed, conn));

        LOG_TRACE<<"remove id="<<conn->get_id();
    }

}