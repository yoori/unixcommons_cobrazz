#ifndef GENERICS_COUNTRYCODEMANIP_HPP
#define GENERICS_COUNTRYCODEMANIP_HPP

#include <cstdint>

#include <String/SubString.hpp>

#include <Generics/Uncopyable.hpp>
#include <Generics/GnuHashTable.hpp>


namespace Generics
{
  class CountryCodeMap : private Uncopyable
  {
  public:
    /**
     * Constructor loads strings with data into hash table.
     * Should not be used without needs.
     */
    CountryCodeMap() /*throw (eh::Exception)*/;

    /**
     * Check
     * @param code SubString point to country code string
     * @return true if str comply ISO3166 standard or
     * ISO3166 extensions or ISO3166 triple country codes.
     * return false if don't comply or str is nul.
     */
    bool
    is_country_code(const String::SubString& code) const throw ();

  private:
    /**
     * Interprets the
     * @param str as the number. Used first 3 chars from str only.
     * Different architectures give different numbers. (Byte order).
     * @return number that encoded in str.
     */
    static
    uint32_t
    get_country_code_(const String::SubString& str) throw ();

    typedef Generics::GnuHashSet<NumericHashAdapter<uint32_t> >
      CountryMap_;
    CountryMap_ country_map_;
  };
}

#endif
