//
// Created by lg on 17-4-21.
//

#include <sys/socket.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <csignal>
#include <cassert>
#include "EventLoop.h"

#include "Log.h"
#include "EventLoopThread.h"
#include "Acceptor.h"
#include "Socket.h"
#include"Event.h"

namespace{
    int createWakeEventfd() {
        int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (evtfd < 0)
        {
            LOG_ERROR << "Failed in eventfd";
            abort();
        }
        return evtfd;
    }

#pragma GCC diagnostic ignored "-Wold-style-cast"
    struct IgnoreSigPipe {
        IgnoreSigPipe() {
            ::signal(SIGPIPE, SIG_IGN);
            LOG_TRACE << "Ignore SIGPIPE";
        }
    };
#pragma GCC diagnostic error "-Wold-style-cast"

    IgnoreSigPipe initObj;
}

namespace net {
    EventLoop::~EventLoop()noexcept {
        assert(!_is_looping);

        _wake_event.detach_from_loop();
        Socket::close(_wake_fd);
    }

     EventLoop::EventLoop()noexcept
            :_is_looping(false)
             ,_is_pending_fns(false)
            ,_wake_fd(createWakeEventfd())
            ,_th_id(std::this_thread::get_id())
            ,_wake_event(this,_wake_fd,true) {

        _wake_event.set_read_cb(std::bind(&EventLoop::handle_wakeup_read, this));
        _wake_event.attach_to_loop();
    }

    void EventLoop::run() {

        assert(!_is_looping);
        assert(_th_id==std::this_thread::get_id());

        bool t=false;
        if(!_is_looping.compare_exchange_strong(t,true))
            return ;

        while(_is_looping){
            LOG_TRACE<<" looping"<<std::endl;

            _poll.wait(-1,_events);

            LOG_TRACE<<"poll "<<_events.size();
            for(auto &e:_events)
                reinterpret_cast<Event*>(e.data.ptr)->handle_event(e.events);

            do_pending_fn();
            LOG_TRACE<<" loop stop"<<std::endl;
        }
        _is_looping=false;
    }

    void EventLoop::stop() {
        bool t=true;

        if(_is_looping.compare_exchange_strong(t,false)) {
            wakeup();
        }
    }

    void EventLoop::wakeup()
    {
        LOG_TRACE;
        uint64_t one = 1;
        ssize_t n = ::write(_wake_fd, &one, sizeof one);
        if (n != sizeof one)
        {
            LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
        }
    }

    void EventLoop::handle_wakeup_read()
    {
        LOG_TRACE;
        uint64_t one = 1;
        ssize_t n = ::read(_wake_fd, &one, sizeof one);
        if (n != sizeof one)
        {
            LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
        }
    }

    void EventLoop::add(Event *e) {
        epoll_event event;
        event.events=e->get_events();
        event.data.ptr=e;
       _poll.add(e->get_fd(),event);
    }

    void EventLoop::update(Event *e) {
        epoll_event event;
        event.events=e->get_events();
        event.data.ptr=e;
        _poll.update(e->get_fd(),event);
    }

    void EventLoop::remove(Event *e) {
        _poll.remove(e->get_fd());
    }

    void EventLoop::run_in_loop(const std::function<void()> &cb) {
        LOG_TRACE;
        if(in_loop_thread()){
            cb();
        }
        else {
            queue_in_loop(cb);
        }
    }

    bool EventLoop::in_loop_thread()const {
        return std::this_thread::get_id()==_th_id;
    }

    void EventLoop::queue_in_loop(const std::function<void()> &cb) {
        {
            std::lock_guard<std::mutex> l(_mu);
            _pending_fns.push_back(cb);
        }

        if(!in_loop_thread()||_is_pending_fns) {
            wakeup();
        }
    }

    void EventLoop::do_pending_fn() {

        std::vector<std::function<void()>>fns;
        _is_pending_fns=true;
        {
            std::lock_guard<std::mutex> l(_mu);
            fns.swap(_pending_fns);
        }

        LOG_TRACE<<fns.size();

        for(auto&f:fns)
            f();
        _is_pending_fns=false;
    }

    void EventLoop::run_after(std::chrono::milliseconds ms, const std::function<void()> &cb) {

    }
}
