//@file  Stream/FlagsSaver.hpp
#ifndef STREAM_FLAGS_SAVER_HPP
#define STREAM_FLAGS_SAVER_HPP

#include <ios>

#include <Generics/Uncopyable.hpp>


namespace Stream
{
  /**
   * Guard for saving std streams state (fmtflags)
   * Provide restore ability.
   */
  class FlagsSaver : Generics::Uncopyable
  {
  public:
    typedef ::std::ios_base StateType;
    typedef ::std::ios_base::fmtflags AspectType;

    /**
     * Constructor
     * @param s Independent from the national characteristics
     * the state of input-output to be saved
     */
    explicit
    FlagsSaver(StateType &s) throw ();

    /**
     * Constructor
     * @param s Independent from the national characteristics
     * the state of input-output to be saved
     * @param a flags to be set on s.
     */
    FlagsSaver(StateType& s, const AspectType& a) throw ();

    /**
     * Destructor restore state of stream
     */
    ~FlagsSaver() throw ();

    /**
     * Restore state of stream
     */
    void
    restore() throw ();

  private:
    StateType& state_;
    const AspectType ASPECT_;
  };
}  // namespace Stream

//////////////////////////////////////////////////////////////////////////
// Inlines implementations
//////////////////////////////////////////////////////////////////////////

namespace Stream
{
  //
  //  class FlagsSaver
  //

  inline
  FlagsSaver::FlagsSaver(StateType &state) throw ()
    : state_(state), ASPECT_(state.flags())
  {
  }

  inline
  FlagsSaver::FlagsSaver(StateType& state, const AspectType& aspect) throw ()
    : state_(state), ASPECT_(state.flags(aspect))
  {
  }

  inline
  void
  FlagsSaver::restore() throw ()
  {
    state_.flags(ASPECT_);
  }

  inline
  FlagsSaver::~FlagsSaver() throw ()
  {
    restore();
  }
}  // namespace Stream

#endif
