// POSIX
#include <fcntl.h>

// STD
#include <iostream>

// THIS
#include <UServerUtils/FileManager/FileReader.hpp>

namespace UServerUtils::FileManager
{

inline FileReader::FileReader(
  const File& file,
  const char delimeter,
  const std::size_t initial_buffer_size,
  const FileManagerPoolPtr& file_manager_pool)
  : file_manager_pool_(file_manager_pool),
    delimeter_(delimeter)
{
  if (file_manager_pool)
  {
    file_ = file;
    if (!file.is_valid())
    {
      Stream::Error stream;
      stream << FNS
             << "File not valid, error code=["
             << file.error_details()
             << "], error message=["
             << file.error_message()
             << "]";
      throw Exception(stream);
    }
  }
  else
  {
#ifdef ENABLE_READ_IFSTREAM
    const std::string path = file.path();
    if (path.empty())
    {
      Stream::Error stream;
      stream << FNS
             << "Path is empty";
      throw Exception(stream);
    }

    auto* const streambuf = ifstream_.rdbuf();
    if (!streambuf)
    {
      Stream::Error stream;
      stream << FNS
             << "streambuf is null";
      throw Exception(stream);
    }
    streambuf->pubsetbuf(nullptr, 0);

    ifstream_.open(path.c_str(), std::ifstream::in | std::ifstream::binary);
    if (!ifstream_.is_open())
    {
      Stream::Error stream;
      stream << FNS
             << "Can't open file="
             << path;
      throw Exception(stream);
    }
#else
    file_ = file;
    if (!file.is_valid())
    {
      Stream::Error stream;
      stream << FNS
             << "File not valid, error code=["
             << file.error_details()
             << "], error message=["
             << file.error_message()
             << "]";
      throw Exception(stream);
    }
#endif
  }

  buffer_.resize(initial_buffer_size <= 2 ? 2 : initial_buffer_size);
  buffer_size_ = buffer_.size() - 1;
  p_buffer_ = buffer_.data();
  buffer_[buffer_size_] = delimeter_;
  end_position_ = p_buffer_ + buffer_size_;
  position_ = end_position_;
}

inline FileReader::State FileReader::load_buffer(
  const File& file,
  std::span<char>& buffer,
  FilePosition& file_position) noexcept
{
  State state = State::Good;
  const std::size_t size = buffer.size();
  int result = 0;
  if (file_manager_pool_)
  {
    result = file_manager_pool_->read(
      file,
      std::string_view(buffer.data(), size),
      file_position);
  }
  else
  {
#ifdef ENABLE_READ_IFSTREAM
    if (ifstream_.read(buffer.data(), size))
    {
      result = size;
    }
    else if (ifstream_.eof())
    {
      result = ifstream_.gcount();
    }
    else
    {
      result = -1;
    }
#else
    result = read(
      file_.fd(),
      buffer.data(),
      size);
#endif
  }

  if (result >= 0) [[likely]]
  {
    file_position += result;
    if (static_cast<std::size_t>(result) < size)
    {
      state = State::Eof;
      buffer = std::span<char>(
        buffer.data(),
        static_cast<std::size_t>(result));
    }
  }
  else
  {
    state = State::Bad;
  }

  return state;
}

inline FileReader& FileReader::getline(std::string& str) noexcept
{
  str.clear();
  if (state_ != State::Good)
  {
    return *this;
  }

  const std::size_t max_size = str.max_size();
  const char* start_position = position_;
  while (true)
  {
    if (*position_ == delimeter_)
    {
      const auto data_size = position_ - start_position;
      if (str.size() + data_size >= max_size)
      {
        state_ = State::Bad;
        return *this;
      }
      str.append(start_position, data_size);

      if (position_ != end_position_) [[likely]]
      {
        position_ += 1;
        return *this;
      }
      else
      {
        if (file_state_ == State::Eof)
        {
          state_ = State::Eof;
          return *this;
        }

        std::span<char> buffer_span(p_buffer_, buffer_size_);
        file_state_ = load_buffer(file_, buffer_span, file_position_);
        if (file_state_ == State::Bad)
        {
          state_ = State::Bad;
          return *this;
        }

        if (buffer_size_ > buffer_span.size())
        {
          if (buffer_span.empty())
          {
            if (str.empty())
            {
              state_ = State::Eof;
            }

            return *this;
          }

          buffer_size_ = buffer_span.size();
          buffer_.resize(buffer_size_ + 1);
          end_position_ = p_buffer_ + buffer_size_;
          *end_position_ = delimeter_;
        }

        position_ = p_buffer_;
        start_position = position_;
        continue;
      }
    }

    position_ += 1;
  }

  return *this;
}

inline FileReader& FileReader::getline(std::string_view& str) noexcept
{
  str = {};
  if (state_ != State::Good)
  {
    return *this;
  }

  const char* start_position = position_;
  while (true)
  {
    if (*position_ != delimeter_) [[likely]]
    {
      position_ += 1;
    }
    else
    {
      if (position_ != end_position_) [[likely]]
      {
        str = std::string_view(start_position, position_ - start_position);
        position_ += 1;

        return *this;
      }
      else
      {
        if (file_state_ == State::Eof)
        {
          if (position_ == start_position)
          {
            state_ = State::Eof;
          }
          else
          {
            str = std::string_view(start_position, position_ - start_position);
          }

          return *this;
        }

        if (start_position != end_position_)
        {
          if (2 * (start_position - p_buffer_) >= buffer_size_)
          {
            const std::size_t size = buffer_size_ - (start_position - p_buffer_);
            std::memcpy(
              p_buffer_,
              start_position,
              size);
            position_ = p_buffer_ + size;
            start_position = p_buffer_;
          }
          else
          {
            try
            {
              buffer_.resize(buffer_size_ + 1 + std::min(1024ul, 1 + buffer_size_ / 2));
              start_position = buffer_.data() + (start_position - p_buffer_);
              p_buffer_ = buffer_.data();
              position_ = p_buffer_ + buffer_size_;
              buffer_size_ = buffer_.size() - 1;
              end_position_ = p_buffer_ + buffer_size_;
              *end_position_ = delimeter_;
            }
            catch (...)
            {
              state_ = State::Bad;
              return *this;
            }
          }
        }
        else
        {
          position_ = p_buffer_;
          start_position = position_;
        }

        std::span<char> buffer_span;
        buffer_span = std::span<char>(
          position_,
          buffer_size_ - (position_ - p_buffer_));
        file_state_ = load_buffer(
          file_,
          buffer_span,
          file_position_);
        if (file_state_ == State::Bad)
        {
          state_ = State::Bad;
          return *this;
        }

        if (buffer_span.empty())
        {
          if (position_ == start_position)
          {
            state_ = State::Eof;
          }
          else
          {
            str = std::string_view(start_position, position_ - start_position);
          }

          return *this;
        }

        std::size_t position_index = position_ - p_buffer_;
        if (buffer_size_ > position_index + buffer_span.size())
        {
          buffer_size_ = position_index + buffer_span.size();
          buffer_.resize(buffer_size_ + 1);
          end_position_ = p_buffer_ + buffer_size_;
          *end_position_ = delimeter_;
        }
      }
    }
  }

  return *this;
}

inline int FileReader::peek() noexcept
{
  static constexpr auto eof = std::char_traits<char>::eof();
  if (state_ != State::Good)
  {
    return eof;
  }

  if (position_ == end_position_)
  {
    if (file_state_ == State::Eof)
    {
      return eof;
    }

    std::span<char> buffer_span(p_buffer_, buffer_size_);
    file_state_ = load_buffer(file_, buffer_span, file_position_);
    if (file_state_ == State::Bad)
    {
      state_ = State::Bad;
      return eof;
    }

    if (buffer_size_ > buffer_span.size())
    {
      if (buffer_span.empty())
      {
        return eof;
      }

      buffer_size_ = buffer_span.size();
      buffer_.resize(buffer_size_ + 1);
      p_buffer_ = buffer_span.data();
      end_position_ = p_buffer_ + buffer_size_;
      *end_position_ = delimeter_;
    }

    position_ = p_buffer_;
  }

  return *position_;
}

inline bool FileReader::eof() const noexcept
{
  return state_ == State::Eof || state_ == State::Bad;
}

inline bool FileReader::bad() const noexcept
{
  return state_ == State::Bad;
}

inline bool FileReader::good() const noexcept
{
  return state_ == State::Good;
}

inline FileReader::operator bool() const noexcept
{
  return state_ == State::Good;
}

} // namespace UServerUtils::FileManager