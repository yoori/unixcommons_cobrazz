#ifndef USERVER_UTILS_SERVER_ACTIVE_OBJECT_HPP
#define USERVER_UTILS_SERVER_ACTIVE_OBJECT_HPP

#include <mutex>
#include <string>
#include <thread>

#include "UServerConfig.hpp"
#include <userver/components/component_list.hpp>

#include <Generics/ActiveObject.hpp>


namespace UServerUtils
{
    /**
     * Adapter for logger into reference countable callback.
     * Forward error reports into logger.
     */
    class Server :
        public virtual Generics::ActiveObject,
        public virtual ReferenceCounting::AtomicImpl
    {
    public:
        Server() /*throw (eh::Exception)*/;

        virtual
        void
        activate_object()
        /*throw (AlreadyActive, Exception, eh::Exception)*/;

        virtual
        void
        deactivate_object()
        /*throw (Exception, eh::Exception)*/;

        virtual
        void
        wait_object()
        /*throw (Exception, eh::Exception)*/;

        virtual
        bool
        active()
        /*throw (eh::Exception)*/;

        void
        add_grpc_server(unsigned short port);

        template <class Comp>
        void
        add_grpc_server_component();

    protected:
        virtual
        ~Server() throw ();

    private:
        std::mutex mutex_;
        std::thread thread_;
        volatile sig_atomic_t state_;
        userver::components::ComponentList component_list_;
        std::string components_config_;
    };
}

//
// INLINES
//

namespace UServerUtils
{
    //
    // Server class
    //

    template <class Comp>
    inline
    void
    Server::add_grpc_server_component()
    {
        component_list_.Append<Comp>();
        components_config_ += "        ";
        components_config_ += Comp::kName;
        components_config_ += ":\n";
        components_config_ += "            task-processor: main-task-processor\n";
    }
}

#endif

