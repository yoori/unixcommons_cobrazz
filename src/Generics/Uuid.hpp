/**
* @file   Uuid.hpp
* @author Denis Badikov
* Uuid creator
*/

#ifndef GENERICS_UUID_HPP
#define GENERICS_UUID_HPP

#include <ios>

#include <Sync/PosixLock.hpp>

#include <String/StringManip.hpp>

#include <Generics/RSA.hpp>


namespace Generics
{
  class Uuid
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidArgument, Exception);

    typedef uint8_t value_type;
    typedef value_type& reference_type;
    typedef const value_type& const_reference_type;
    typedef value_type* iterator;
    typedef const value_type* const_iterator;
    typedef ssize_t difference_type;
    typedef size_t size_type;


    // random number based
    static
    Uuid
    create_random_based() noexcept;

    Uuid() noexcept;

    explicit
    Uuid(const char* str, bool padding = true)
      /*throw (eh::Exception, Exception, InvalidArgument)*/;

    explicit
    Uuid(const String::SubString& str, bool padding = true)
      /*throw (eh::Exception, Exception, InvalidArgument)*/;

    explicit
    Uuid(std::istream& istr)
      /*throw (eh::Exception, Exception, InvalidArgument)*/;

    template <typename ByteInputIterator>
    Uuid(ByteInputIterator first, ByteInputIterator last)
      /*throw (eh::Exception, Exception, InvalidArgument)*/;

    bool
    operator ==(const Uuid& rhs) const noexcept;

    bool
    operator !=(const Uuid& rhs) const noexcept;

    bool
    operator <(const Uuid& rhs) const noexcept;

    bool
    operator >(const Uuid& rhs) const noexcept;

    bool
    operator <=(const Uuid& rhs) const noexcept;

    bool
    operator >=(const Uuid& rhs) const noexcept;

    bool
    is_null() const noexcept;

    std::string
    to_string(bool padding = true) const
      /*throw (eh::Exception)*/;

    static
    size_type
    size() noexcept;

    static
    size_type
    encoded_size(bool padding = true) noexcept;

    iterator
    begin() noexcept;

    const_iterator
    begin() const noexcept;

    iterator
    end() noexcept;

    const_iterator
    end() const noexcept;

    void
    swap(Uuid& rhs) noexcept;

    unsigned long
    hash() const noexcept;

  private:
    static const size_type DATA_SIZE = 16;
    typedef value_type DataType[DATA_SIZE];

    template <typename Iterator>
    Iterator
    construct_(Iterator begin, Iterator end, bool padding)
      /*throw (eh::Exception, Exception, InvalidArgument)*/;

    void
    construct_(const String::SubString& str, bool padding)
      /*throw (eh::Exception, Exception, InvalidArgument)*/;

    union
    {
      DataType data_;
      uint64_t hash_[2];
    };
  }
# ifdef __GNUC__
  __attribute__ ((packed))
# endif
  ;

  std::ostream&
  operator <<(std::ostream& ostr, const Uuid& uuid) noexcept;
  std::istream&
  operator >>(std::istream& istr, Uuid& uuid) noexcept;

  template <typename Hash>
  void
  hash_add(Hash& hash, const Uuid& value) noexcept;


  class SignedUuidGenerator;
  class SignedUuidVerifier;

  /**
   * Class containing Uuid, it's signature and four data bits
   * May be constructed only by SignedUuidGenerator or SignedUuidVerifier
   */
  class SignedUuid
  {
  public:
    /**
     * Returns contained uuid
     * @return contained uuid
     */
    const Uuid&
    uuid() const noexcept;

    /**
     * Returns contained data bits
     * @return contained data bits
     */
    uint8_t
    data() const noexcept;

    /**
     * String representation of Uuid and its signature. It can be parsed
     * by SignedUuidVerifier to create SignedUuid object.
     * @return string representation of the signed uuid
     */
    const std::string&
    str() const noexcept;


  private:
    /**
     * Constructor
     * @param uuid uuid to hold
     * @param data data bits
     * @param sign its signature (base64-encoded)
     */
    SignedUuid(const Uuid& uuid, uint8_t data,
      const String::SubString& sign) /*throw (eh::Exception)*/;

    Uuid uuid_;
    uint8_t data_;
    std::string str_;

    friend class SignedUuidGenerator;
    friend class SignedUuidVerifier;
    friend class SignedUuidProbe;
  };

  /**
   * Generator of SignedUuids
   * Requires private RSA key for signing
   */
  class SignedUuidGenerator
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    /**
     * Constructor
     * Reads the private RSA key
     * @param private_key name of ASN1 file containing private RSA key
     */
    SignedUuidGenerator(const char* private_key)
      /*throw (eh::Exception)*/;

    /**
     * Generates random uuid and signs it.
     * @param data optional data bits
     * @return randomly generated SignedUuid
     */
    SignedUuid
    generate(uint8_t data = 0) const /*throw (eh::Exception, Exception)*/;

    /**
     * Signs the supplied uuid.
     * @param uuid uuid to sign
     * @param data optional data bits
     * @return randomly generated SignedUuid
     */
    SignedUuid
    sign(const Uuid& uuid, uint8_t data = 0) const
      /*throw (eh::Exception, Exception)*/;

  private:
    RSAKey<true> key_;
    const unsigned SIZE_;
  };

  /**
   * Verifies if a string represents SignedUuid
   * Requires public RSA key for signature verifying.
   */
  class SignedUuidVerifier
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    /**
     * Constructor
     * Reads the public RSA key
     * @param public_key name of ASN1 file containing public RSA key
     */
    SignedUuidVerifier(const char* public_key)
      /*throw (eh::Exception)*/;

    /**
     * Verifies if a string represents SignedUuid and creates the object
     * @param uuid_str signed Uuid string
     * @param data_expected if data bits are expected or not
     * @return SignedUuid parsed from the string representation
     */
    SignedUuid
    verify(const String::SubString& uuid_str,
      bool data_expected = false) const /*throw (eh::Exception, Exception)*/;

  private:
    RSAKey<false> key_;
    const unsigned SIZE_;
  };

  /**
   * Special SignedUuid creator returning the same _not signed_ Uuid.
   * Designed for special Probe Uuid.
   */
  class SignedUuidProbe
  {
  public:
    explicit
    SignedUuidProbe(const Uuid& probe) noexcept;

    SignedUuid
    construct() const /*throw (eh::Exception)*/;

  private:
    SignedUuid probe_;
  };
}

namespace Generics
{
  //
  // Uuid class
  //

  template <typename ByteInputIterator>
  Uuid::Uuid(ByteInputIterator first, ByteInputIterator last)
    /*throw (eh::Exception, Exception, InvalidArgument)*/
  {
    size_t i = 0;
    for (; i < DATA_SIZE && first != last; ++i)
    {
      data_[i] = static_cast<value_type>(*first++);
    }
    if (i != DATA_SIZE)
    {
      Stream::Error ostr;
      ostr << FNS << "invalid input Uuid iterator pair, must span 16 bytes";
      throw InvalidArgument(ostr);
    }
  }

  inline
  bool
  Uuid::operator ==(const Uuid& rhs) const noexcept
  {
    for (size_t i = 0; i < DATA_SIZE; i++)
    {
      if (data_[i] != rhs.data_[i])
      {
        return false;
      }
    }
    return true;
  }

  inline
  bool
  Uuid::operator !=(const Uuid& rhs) const noexcept
  {
    return !operator ==(rhs);
  }

  inline
  bool
  Uuid::operator <(const Uuid& rhs) const noexcept
  {
    for (size_t i = 0; i < DATA_SIZE; i++)
    {
      if (data_[i] < rhs.data_[i])
      {
        return true;
      }
      if (data_[i] > rhs.data_[i])
      {
        break;
      }
    }
    return false;
  }

  inline
  bool
  Uuid::operator >(const Uuid& rhs) const noexcept
  {
    return rhs < *this;
  }

  inline
  bool
  Uuid::operator <=(const Uuid& rhs) const noexcept
  {
    return !operator >(rhs);
  }

  inline
  bool
  Uuid::operator >=(const Uuid& rhs) const noexcept
  {
    return !operator <(rhs);
  }

  inline
  bool
  Uuid::is_null() const noexcept
  {
    for (size_t i = 0; i < DATA_SIZE; i++)
    {
      if (data_[i] != 0)
      {
        return false;
      }
    }

    return true;
  }

  inline
  Uuid::size_type
  Uuid::size() noexcept
  {
    return DATA_SIZE;
  }

  inline
  Uuid::size_type
  Uuid::encoded_size(bool padding) noexcept
  {
    return String::StringManip::base64mod_encoded_size(DATA_SIZE, padding);
  }

  inline
  Uuid::iterator
  Uuid::begin() noexcept
  {
    return data_;
  }

  inline
  Uuid::const_iterator
  Uuid::begin() const noexcept
  {
    return data_;
  }

  inline
  Uuid::iterator
  Uuid::end() noexcept
  {
    return data_ + DATA_SIZE;
  }

  inline
  Uuid::const_iterator
  Uuid::end() const noexcept
  {
    return data_ + DATA_SIZE;
  }

  inline
  void
  Uuid::swap(Uuid& rhs) noexcept
  {
    DataType data;
    std::copy(begin(), end(), data);
    std::copy(rhs.begin(), rhs.end(), data_);
    std::copy(data, data + sizeof(data), rhs.data_);
  }

  inline
  unsigned long
  Uuid::hash() const noexcept
  {
    return hash_[1];
  }


  template <typename Hash>
  void
  hash_add(Hash& hash, const Uuid& value) noexcept
  {
    hash.add(value.begin(), value.size());
  }


  //
  // SignedUuid class
  //

  inline
  const Uuid&
  SignedUuid::uuid() const noexcept
  {
    return uuid_;
  }

  inline
  uint8_t
  SignedUuid::data() const noexcept
  {
    return data_;
  }

  inline
  const std::string&
  SignedUuid::str() const noexcept
  {
    return str_;
  }
} // namespace Generics


#endif
