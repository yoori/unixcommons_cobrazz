#ifndef USERVER_FILEMANAGER_FILEREADER_HPP
#define USERVER_FILEMANAGER_FILEREADER_HPP

// STD
#include <fstream>
#include <vector>
#include <span>

// THIS
#include <UServerUtils/FileManager/File.hpp>
#include <UServerUtils/FileManager/FileManagerPool.hpp>

namespace UServerUtils::FileManager
{

class FileReader final
{
private:
  using Buffer = std::vector<char>;
  using FilePosition = off_t;

  enum class State : std::uint8_t
  {
    Good = 0,
    Bad,
    Eof
  };

public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit FileReader(
    const File& file,
    const char delimeter = '\n',
    const std::size_t initial_buffer_size = 4096,
    const FileManagerPoolPtr& file_manager_pool = {});

  FileReader(const FileReader&) = delete;
  FileReader(FileReader&&) = delete;
  FileReader& operator=(const FileReader&) = delete;
  FileReader& operator=(FileReader&&) = delete;

  ~FileReader() = default;

  FileReader& getline(std::string& str) noexcept;

  FileReader& getline(std::string_view& str) noexcept;

  int peek() noexcept;

  bool eof() const noexcept;

  bool bad() const noexcept;

  bool good() const noexcept;

  operator bool() const noexcept;

private:
  State load_buffer(
    const File& file,
    std::span<char>& buffer,
    FilePosition& file_position) noexcept;

private:
  const FileManagerPoolPtr file_manager_pool_;

  File file_;

#ifdef ENABLE_READ_IFSTREAM
  std::ifstream ifstream_;
#endif

  const char delimeter_ = '\n';

  Buffer buffer_;

  std::size_t buffer_size_ = 0;

  char* p_buffer_ = nullptr;

  char* position_ = nullptr;

  char* end_position_ = nullptr;

  State state_ = State::Good;

  FilePosition file_position_ = 0;

  State file_state_ = State::Good;
};

} // namespace UServerUtils::FileManager

#include <UServerUtils/FileManager/FileReader.ipp>

#endif //USERVER_FILEMANAGER_FILEREADER_HPP