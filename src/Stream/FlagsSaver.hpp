#pragma once

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
    using StateType = ::std::ios_base;
    using AspectType = ::std::ios_base::fmtflags;

    /**
     * Constructor
     * @param s Independent from the national characteristics
     * the state of input-output to be saved
     */
    explicit FlagsSaver(StateType &s) noexcept;

    /**
     * Constructor
     * @param s Independent from the national characteristics
     * the state of input-output to be saved
     * @param a flags to be set on s.
     */
    FlagsSaver(StateType& s, const AspectType& a) noexcept;

    /**
     * Destructor restore state of stream
     */
    ~FlagsSaver() noexcept;

    /**
     * Restore state of stream
     */
    void restore() noexcept;

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

  inline FlagsSaver::FlagsSaver(StateType &state) noexcept
    : state_(state), ASPECT_(state.flags())
  {
  }

  inline FlagsSaver::FlagsSaver(StateType& state, const AspectType& aspect) noexcept
    : state_(state), ASPECT_(state.flags(aspect))
  {
  }

  inline void FlagsSaver::restore() noexcept
  {
    state_.flags(ASPECT_);
  }

  inline FlagsSaver::~FlagsSaver() noexcept
  {
    restore();
  }
}  // namespace Stream
