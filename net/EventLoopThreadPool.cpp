#include "EventLoopThreadPool.h"

void net::EventLoopThreadPool::run()
{
    for (auto &t:_threads)
        t.run();
}

void net::EventLoopThreadPool::stop()
{
    for (auto &t:_threads)
        t.stop();
}

void net::EventLoopThreadPool::join()
{
    for (auto &t:_threads)
        t.join();
}

net::EventLoopThreadPool::EventLoopThreadPool(EventLoop*loop,size_t threadNum)
        :_loop(loop)
        ,_threads(threadNum)
          , _loop_index(0)
{

}

size_t net::EventLoopThreadPool::next_loop_index()
{
    size_t t = _loop_index++;
    _loop_index %= _threads.size();
    return t;
}

net::EventLoop *net::EventLoopThreadPool::get_next_loop()
{
    if (_threads.empty()) {
        return _loop;
    }
    else{
        return _threads[next_loop_index()].get_loop();
    }
}
