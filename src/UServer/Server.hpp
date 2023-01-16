#ifndef USERVER_SERVER_ACTIVE_OBJECT_HPP
#define USERVER_SERVER_ACTIVE_OBJECT_HPP

#include <memory>
#include <string>
#include <thread>

#include "UServerConfig.hpp"
#include <userver/components/component_list.hpp>

#include <Generics/ActiveObject.hpp>

#include <Logger/Logger.hpp>


namespace UServer
{
  /**
   * Adapter for logger into reference countable callback.
   * Forward error reports into logger.
   */
  class Server : public virtual Generics::ActiveObject
  {
  public:
    Server(unsigned short grpc_port) /*throw (eh::Exception)*/;

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

    template <class Comp>
    void
    add_grpc_component();

  protected:
    virtual
    ~Server() throw ();

  private:
    std::thread thread_;
    volatile sig_atomic_t state_;
    std::shared_ptr<userver::components::ComponentList> component_list_;
    std::string components_config_;
  };
}

//
// INLINES
//

namespace UServer
{
  //
  // Server class
  //

  template <class Comp>
  inline
  void
  Server::add_grpc_component()
  {
    component_list_->Append<Comp>();
    components_config_ += "        ";
    components_config_ += Comp::kName;
    components_config_ += ":\n";
    components_config_ += "            task-processor: main-task-processor\n";
  }
}

#endif

