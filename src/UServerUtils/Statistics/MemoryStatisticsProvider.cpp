// STD
#include <cstdint>
#include <memory>
#include <iostream>

// POSIX
#include <jemalloc/jemalloc.h>
#include <malloc.h>
#include <stdio.h>

// BOOST
#include <boost/exception/diagnostic_information.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

// THIS
#include <Generics/Function.hpp>
#include <Stream/MemoryStream.hpp>
#include <UServerUtils/Statistics/MemoryStatisticsProvider.hpp>

namespace UServerUtils::Statistics
{

void MemoryStatisticsProvider::write(Writer& writer)
{
  const auto info_malloc = get_malloc_info();
  writer.ValueWithLabels(
    info_malloc.total,
    {LabelView{"malloc", "total_memory"}});
  writer.ValueWithLabels(
    info_malloc.in_use,
    {LabelView{"malloc", "in_use_memory"}});

  const auto info_jemalloc = get_jemalloc_info();
  writer.ValueWithLabels(
    info_jemalloc.allocated,
    {LabelView{"jemalloc", "allocated"}});
  writer.ValueWithLabels(
    info_jemalloc.active,
    {LabelView{"jemalloc", "active"}});
  writer.ValueWithLabels(
    info_jemalloc.mapped,
    {LabelView{"jemalloc", "mapped"}});
  writer.ValueWithLabels(
    info_jemalloc.metadata,
    {LabelView{"jemalloc", "metadata"}});
  writer.ValueWithLabels(
    info_jemalloc.resident,
    {LabelView{"jemalloc", "resident"}});
  writer.ValueWithLabels(
    info_jemalloc.retained,
    {LabelView{"jemalloc", "retained"}});
}

std::string MemoryStatisticsProvider::name()
{
  return "memory_usage";
}

MemoryStatisticsProvider::MallocInfo
MemoryStatisticsProvider::get_malloc_info()
{
  char* buffer = nullptr;
  std::size_t buffer_size = 0;

  auto deleter = [] (FILE* f) {
    fclose(f);
  };

  std::unique_ptr<FILE, decltype(deleter)> memstream(
    open_memstream(&buffer, &buffer_size),
    deleter);
  if (!memstream)
  {
    Stream::Error ostr;
    ostr << FNS
         << "open_memstream is failed";
    throw Exception(ostr);
  }

  if (malloc_info(0, memstream.get()) == -1)
  {
    Stream::Error ostr;
    ostr << FNS
         << "malloc_info is failed";
    throw Exception(ostr);
  }

  fflush(memstream.get());
  if (buffer_size == 0)
  {
    Stream::Error ostr;
    ostr << FNS
         << "buffer is empty";
    throw Exception(ostr);
  }

  try
  {
    std::stringstream stream;
    stream << std::string_view(buffer, buffer_size);
    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(stream, pt);

    std::uint64_t free_mem = 0;
    std::uint64_t total_mem = 0;

    const auto total_range = pt.get_child("malloc").equal_range("total");
    auto it = total_range.first;
    auto it_end = total_range.second;
    for (; it != it_end; ++it)
    {
      const auto& subtree = it->second;
      const std::string type = subtree.get<std::string>("<xmlattr>.type");
      if (type == "rest" || type == "fast")
      {
        free_mem += subtree.get<std::size_t>("<xmlattr>.size");
      }
      else if (type == "mmap")
      {
        total_mem += subtree.get<std::size_t>("<xmlattr>.size");
      }
    }

    const auto system_range = pt.get_child("malloc").equal_range("system");
    it = system_range.first;
    it_end = system_range.second;
    for (; it != it_end; ++it)
    {
      const auto& subtree = it->second;
      const std::string type = subtree.get<std::string>("<xmlattr>.type");
      if (type == "current")
      {
        total_mem += subtree.get<std::size_t>("<xmlattr>.size");
      }
    }

    MallocInfo malloc_info;
    malloc_info.total = total_mem;
    malloc_info.in_use = total_mem - free_mem;

    return malloc_info;
  }
  catch (const boost::exception& exc)
  {
    const std::string error_info = diagnostic_information(exc);
    Stream::Error ostr;
    ostr << FNS
         << error_info;
    throw Exception(ostr);
  }
}

MemoryStatisticsProvider::JemallocInfo
MemoryStatisticsProvider::get_jemalloc_info()
{
  size_t sz = sizeof(size_t);
  size_t allocated = 0;
  size_t active = 0;
  size_t metadata = 0;
  size_t resident = 0;
  size_t mapped = 0;
  size_t retained = 0;

  JemallocInfo jemalloc_info;
  if (mallctl("stats.allocated", &allocated, &sz, nullptr, 0) == 0)
  {
    jemalloc_info.allocated = allocated;
  }
  if (mallctl("stats.active", &active, &sz, nullptr, 0) == 0)
  {
    jemalloc_info.active = active;
  }
  if (mallctl("stats.metadata", &metadata, &sz, nullptr, 0) == 0)
  {
    jemalloc_info.metadata = metadata;
  }
  if (mallctl("stats.resident", &resident, &sz, nullptr, 0) == 0)
  {
    jemalloc_info.resident = resident;
  }
  if (mallctl("stats.mapped", &mapped, &sz, nullptr, 0) == 0)
  {
    jemalloc_info.mapped = mapped;
  }
  if (mallctl("stats.retained", &retained, &sz, nullptr, 0) == 0)
  {
    jemalloc_info.retained = retained;
  }

  return jemalloc_info;
}

} // namespace UServerUtils::Statistics