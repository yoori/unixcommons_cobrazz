// @file Generics/MMap.hpp
#ifndef GENERICS_MMAP_HPP
#define GENERICS_MMAP_HPP

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <eh/Exception.hpp>

#include <Generics/Uncopyable.hpp>


namespace Generics
{
  /**
   * Memory mapping for a file
   */
  class MMap : private Uncopyable
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    /**
     * Constructor
     * Maps opened file into memory
     * @param fd file to map, descriptor will not be closed by the object
     * @param size size to map (zero - from offset till the end)
     * @param offset starting offset in file
     * @param mmap_prot protection type to pass to mmap(2)
     * @param mmap_flags flags to pass to mmap(2)
     */
    explicit
    MMap(int fd, size_t size = 0, off_t offset = 0,
      int mmap_prot = PROT_READ,
      int mmap_flags = MAP_PRIVATE | MAP_NORESERVE | MAP_FILE)
      /*throw (eh::Exception, Exception)*/;

    /**
     * Constructor make anonymous shared memory region
     * available for read/write operation.
     * @param preferrable_address Hint address to allocate shared memory,
     * actual place where the object will mapped may have another address
     * @param size size to mapped shared memory
     */
    explicit
    MMap(void* preferrable_address, std::size_t size)
      /*throw (eh::Exception, Exception)*/;

    ~MMap() throw ();

    /**
     * @return address of the mapped region
     */
    void*
    memory() const throw ();
    /**
     * @return size of the mapped region
     */
    size_t
    length() const throw ();

  protected:
    MMap() throw ();

    void
    map_(int fd, void* preferrable_address, size_t size, off_t offset,
      int mmap_prot, int mmap_flags) /*throw (eh::Exception, Exception)*/;

  private:
    void* memory_;
    size_t length_;
  };

  /**
   * Memory mapping for a file
   * Holds file descriptor and closes it (in all cases)
   */
  class MMapFile : protected MMap
  {
  public:
    /**
     * Constructor
     * Opens the file and maps it into memory
     * @param filename file to open
     * @param size size to map (zero - from offset till the end)
     * @param offset starting offset in file
     * @param flags flags to pass to open(2)
     * @param mmap_prot protection type to pass to mmap(2)
     * @param mmap_flags flags to pass to mmap(2)
     */
    explicit
    MMapFile(const char* filename, size_t size = 0, off_t offset = 0,
      int flags = O_RDONLY, int mmap_prot = PROT_READ,
      int mmap_flags = MAP_PRIVATE | MAP_NORESERVE | MAP_FILE)
      /*throw (eh::Exception, Exception)*/;

    /**
     * Constructor
     * Maps opened file into memory
     * @param fd file to map, descriptor will be closed by the object
     * @param size size to map (zero - from offset till the end)
     * @param offset starting offset in file
     * @param mmap_prot protection type to pass to mmap(2)
     * @param mmap_flags flags to pass to mmap(2)
     */
    explicit
    MMapFile(int fd, size_t size = 0, off_t offset = 0,
      int mmap_prot = PROT_READ,
      int mmap_flags = MAP_PRIVATE | MAP_NORESERVE | MAP_FILE)
      /*throw (eh::Exception, Exception)*/;

    /**
     * Destructor
     * Unmaps file and closes it
     */
    ~MMapFile() throw ();

    using MMap::memory;
    using MMap::length;

    int
    file_descriptor() const throw ();

  private:
    int fd_;
  };
}

#endif
