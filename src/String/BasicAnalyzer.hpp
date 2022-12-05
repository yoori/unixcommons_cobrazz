/**
 * @file   String/BasicAnalyzer.hpp
 * @author Anna Ignatenkova
 */

#ifndef STRING_BASIC_ANALIZER_HPP
#define STRING_BASIC_ANALIZER_HPP

#include <istream>
#include <ostream>
#include <list>

#include <eh/Exception.hpp>


namespace String
{
  namespace SequenceAnalyzer
  {
    /**
     * Exception that raised by prepared standard translator
     */
    DECLARE_EXCEPTION(BasicAnalyzerException, eh::DescriptiveException);

    /**
     * Translate through a prepared standard translator
     * @param istr Input data stream
     * @param ostr Stream to store translation results
     */
    void
    interprete_base_sequence(std::istream& istr, std::ostream& ostr)
      /*throw (BasicAnalyzerException, eh::Exception)*/;

    /**
     * Translate through a prepared standard translator
     * @param istr Input data stream
     * @param ret_list Translation results stored in the form of a
     * std::list
     */
    void
    interprete_base_sequence(std::istream& istr,
      std::list<std::string>& ret_list)
      /*throw (BasicAnalyzerException, eh::Exception)*/;
  } // namespace SequenceAnalyzer
}

#endif
