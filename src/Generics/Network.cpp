#include <sys/socket.h>
#include <arpa/inet.h>

#include <eh/Errno.hpp>

#include <Generics/Network.hpp>
#include <Generics/Function.hpp>

#include <Stream/MemoryStream.hpp>


namespace Generics
{
  namespace Network
  {
    //
    // Resolver class
    //

    void
    Resolver::get_host_by_name(const char* host_name,
      hostent& addresses, char* buf, size_t buf_size)
      /*throw (Exception)*/
    {
      if (!host_name)
      {
        Stream::Error ostr;
        ostr << FNS << "NULL host name";
        throw InvalidArgument(ostr);
      }

      hostent* result;
      int error;
      if (gethostbyname_r(host_name, &addresses, buf, buf_size,
        &result, &error))
      {
        eh::throw_errno_exception<GetHostByNameFailed>(error,
          FNE, "gethostbyname_r failed on host name '", host_name, "'");
      }

      if (!result)
      {
        Stream::Error ostr;
        ostr << FNS << "host name '" << host_name << "' is unknown to DNS";
        throw UnresolvableAddress(ostr);
      }
    }


    //
    // LocalInterfaces class
    //

    LocalInterfaces::LocalInterfaces() /*throw (eh::Exception, Exception)*/
      : addresses_(0)
    {
      if (getifaddrs(&addresses_))
      {
        eh::throw_errno_exception<Exception>(FNE,
          "failed to enum interfaces");
      }
    }

    LocalInterfaces::~LocalInterfaces() throw ()
    {
      freeifaddrs(addresses_);
    }


    //
    // IsLocalInterface class
    //

    IsLocalInterface::IsLocalInterface() /*throw (eh::Exception)*/
    {
      LocalInterfaces local_interfaces;
      local_interfaces.list_all(local_addresses_, ip_address);
    }

    bool
    IsLocalInterface::check_host_name(const char* host_name) const
      /*throw (eh::Exception, InvalidArgument, GetHostByNameFailed,
        UnresolvableAddress)*/
    {
      hostent addresses;
      char buf[2048];
      get_host_by_name(host_name, addresses, buf, sizeof(buf));
      const in_addr* address;
      for (int i = 0; (address =
        reinterpret_cast<const in_addr*>(addresses.h_addr_list[i])); i++)
      {
        if (local_addresses_.find(address->s_addr) != local_addresses_.end())
        {
          return true;
        }
      }

      return false;
    }

    uint32_t
    IsLocalInterface::ip_address(const sockaddr_in* address) throw ()
    {
      return address->sin_addr.s_addr;
    }
  }
}
