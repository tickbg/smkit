//
// net_service.h
// Copyright (C) 2015  Emil Penchev, Bulgaria


#ifndef NET_SERVICE_H_
#define NET_SERVICE_H_

#include <set>
#include <list>
#include <string>
#include <memory>

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

/// Represents an object handling incoming TCP connections via socket.
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

typedef boost::shared_ptr<tcp_listener> tcp_listener_ptr;

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

private:
    Listener listener_;
};

/// Represents a network service handling incoming TCP connections via socket object.
/// It's a container factory for tcp_listener objects.
class net_service
{
public:
    net_service();
    virtual ~net_service() {}

    /// Entry point for every network service where a new connection is accepted and handled.
    virtual void handle_accept(tcp_socket_ptr sock);

    /// Factory method.
    /// It's very important for subclasses to overload this method,
    /// objects from this class will not be created directly but from a reg_factory<> instance.
    static net_service* create_object() { return NULL; }

protected:
    /// Internal method to setup listeners map.
    virtual void init_listeners();

    std::map<thread_id_t, tcp_listener_ptr> listeners_;  // tcp_listener objects with the given implementation.
};

/// Main system class, does complete system initialization and provides access to all core objects.
class snode_core
{
private:
    unsigned                                current_thread_idx_;    /// holds index of the current thread to be used to schedule I/O
    io_event_threadpool*                    ev_threadpool_;         /// handling all I/O and event messaging tasks.
    sys_processor_threadpool*               sys_threadpool_;        /// handling all heavy computing tasks.
    std::list<tcp_acceptor_ptr>             acceptors_;             /// socket acceptors listening for incoming connections.
    app_config                              config_;                /// master configuration.
    std::map<unsigned short, net_service*>  services_;              /// net port -> net_service map association

public:
    /// server_controller is a singleton, can be accessed only with this method.
    static snode_core& instance()
    {
        static snode_core s_core;
        return s_core;
    }

    /// Factory for registering all the server handler classes.
    typedef reg_factory<net_service> service_factory;

    /// Main entry point of the system, read configuration and runs the system.
    void run();

    /// Stop server engine and cancel all tasks waiting to be executed.
    void stop();

    sys_processor_threadpool& processor_threadpool();

    io_event_threadpool& event_threadpool();

private:
    /// internal initialization structure
    void init();

    /// boost acceptor handler callback function
    void accept_handler(tcp_socket_ptr socket, tcp_acceptor_ptr acceptor, const error_code_t& err);

    snode_core();
    ~snode_core();
};

}

#endif /* NET_SERVICE_H_ */