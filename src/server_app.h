//
// server_app.h
// Copyright (C) 2015  Emil Penchev, Bulgaria


#ifndef SERVER_APP_H_
#define SERVER_APP_H_

#include <set>
#include <list>
#include <string>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>

#include "threadpool.h"
#include "reg_factory.h"
#include "config_reader.h"

namespace snode
{

typedef boost::system::error_code error_code_t;
typedef boost::shared_ptr<boost::asio::ip::tcp::socket> tcp_socket_ptr;
typedef boost::shared_ptr<boost::asio::ip::tcp::acceptor> tcp_acceptor_ptr;

/// Represents a server instance handling incoming TCP connection via socket object.
class tcp_listener
{
public:
    void handle_accept(tcp_socket_ptr sock)
    {
        func_(this, sock);
    }

protected:
    typedef void (*accept_func)(tcp_listener*, tcp_socket_ptr);
    tcp_listener(accept_func func) : func_(func)
    {
    }

private:
    accept_func func_;
};

/// A template class that servers as a implementation wrapper for a tcp_listener.
/// Template type Listener is the actual implementation for a tcp_listener.
template <typename Listener>
class tcp_listener_impl : public tcp_listener
{
public:
    /// Default constructor , (lr) is the listener to be called when a new TCP connection/socket is accepted.
    tcp_listener_impl(Listener lr) : tcp_listener(&tcp_listener_impl::handle_accept_impl), listener_(lr)
    {
    }

    static void handle_accept_impl(tcp_listener* base, tcp_socket_ptr sock)
    {
        tcp_listener_impl<Listener>* lr(static_cast<tcp_listener_impl<Listener>*>(base));
        lr->listener_.handle_accept(sock);
    }

    /// Factory method
    static tcp_listener* create_object()
    {
        Listener lr;
        return new tcp_listener_impl<Listener>(lr);
    }

private:
    Listener listener_;
};

/// Main system class, does complete system initialization and provides access to all core objects.
class server_app
{
private:
    unsigned                                current_thread_idx_;    /// holds index of the current thread to be used to schedule I/O
    io_event_threadpool*                    ev_threadpool_;         /// handling all I/O and event messaging tasks.
    sys_processor_threadpool*               sys_threadpool_;        /// handling all heavy computing tasks.
    std::list<tcp_acceptor_ptr>             acceptors_;             /// socket acceptors listening for incoming connections.
    app_config                              config_;                /// master configuration.
    sys_processor_threadpool::task_queue_t  task_queue_;
    std::map<thread_id_t, std::map<unsigned short, std::pair<tcp_listener*, std::string>>> listeners_;   /// threads -> tcp_listeners map association

public:
    /// server_controller is a singleton, can be accessed only with this method.
    static server_app& instance()
    {
        static server_app s_app;
        return s_app;
    }

    /// Factory for registering all the server handler classes.
    typedef reg_factory<tcp_listener> service_factory;

    /// Main entry point of the system, read configuration and runs the system.
    void run();
    sys_processor_threadpool& processor_threadpool();
    io_event_threadpool& event_threadpool();

private:
    /// internal initialization structure
    void init();

    /// boost acceptor handler callback function
    void accept_handler(tcp_socket_ptr socket, tcp_acceptor_ptr acceptor, const error_code_t& err);

    server_app() : current_thread_idx_(0), ev_threadpool_(NULL), sys_threadpool_(NULL)
    {
    }

    ~server_app()
    {
        delete ev_threadpool_;
    }
};

}

#endif /* SERVER_APP_H_ */
