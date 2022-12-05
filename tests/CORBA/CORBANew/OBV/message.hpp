#ifndef MESSAGE_HPP
#define MESSAGE_HPP

class MessageHolder_i :
#ifdef TESTING_CUSTOM
  public MessageHolder,
#else
  public OBV_MessageHolder,
#endif
  public virtual ::CORBA::DefaultValueRefCountBase
{
public:
  MessageHolder_i();
  MessageHolder_i(const char* init_message);
  virtual char*
  get_message();

#ifdef TESTING_CUSTOM
    virtual void
    message (char *val);
    virtual void
    message (const char *val);
    virtual void
    message (const ::CORBA::String_var &val);
    virtual const char *
    message (void) const;

    virtual void
    marshal(::CORBA::DataOutputStream* os)
    {
      std::cerr << "Marshalling\n";
      os->write_string(message());
    }

    virtual void
    unmarshal(::CORBA::DataInputStream* is)
    {
      std::cerr << "Unmarshalling\n";
      CORBA::String_var str = is->read_string();
      message(str);
    }

  private:
    std::string message_;
#endif

#ifdef ORB_TAO
  protected:
  virtual ::CORBA::ValueBase*
  _copy_value();
#endif
};


class MessageHolderFactory :
  public virtual ::CORBA::ValueFactoryBase
{
private:
  virtual ::CORBA::ValueBase*
  create_for_unmarshal();
};


MessageHolder_i::MessageHolder_i()
{
}

MessageHolder_i::MessageHolder_i(const char* init_message)
#ifdef TESTING_CUSTOM
  : message_(init_message)
#else
  : OBV_MessageHolder(init_message)
#endif
{
}

char*
MessageHolder_i::get_message()
{
  return CORBA::string_dup(message());
}

#ifdef TESTING_CUSTOM
void
MessageHolder_i::message (char *val)
{
  ::CORBA::String_var tmp(val);
  message_ = val;
}
void
MessageHolder_i::message (const char *val)
{
  message_ = val;
}
void
MessageHolder_i::message (const ::CORBA::String_var &val)
{
  message_ = val;
}
const char *
MessageHolder_i::message (void) const
{
  return message_.c_str();
}
#endif

#ifdef ORB_TAO
::CORBA::ValueBase*
MessageHolder_i::_copy_value()
{
  _add_ref();
  return this;
}
#endif

::CORBA::ValueBase*
MessageHolderFactory::create_for_unmarshal()
{
  return new MessageHolder_i;
}

#endif
