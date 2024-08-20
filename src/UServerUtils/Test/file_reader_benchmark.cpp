// STD
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Function.hpp>
#include <UServerUtils/FileManager/File.hpp>
#include <UServerUtils/FileManager/FileReader.hpp>

class Application final
{
public:
  Application(
    const std::size_t number_line,
    const std::size_t length_line_min,
    const std::size_t length_line_max,
    const std::string& file_path,
    const std::size_t size_buffer)
    : number_line_(number_line),
      file_path_(file_path),
      size_buffer_(size_buffer),
      length_line_min_(length_line_min),
      length_line_max_(length_line_max)
  {
  }

  int run()
  {
    std::cout << "Start creation test file" << std::endl;
    create_file(
      number_line_,
      length_line_min_,
      length_line_max_,
      file_path_);
    std::cout << "Test file is created success\n" << std::endl;

    {
      std::cout << "Start benchmark istream" << std::endl;

      const auto time_start = std::chrono::high_resolution_clock::now();
      const auto number_line = benchmark_istream();
      const auto time_end = std::chrono::high_resolution_clock::now();
      double elapsed_time_ms = std::chrono::duration<double, std::milli>(time_end - time_start).count();

      std::cout << "Benchmark istream is success finish: \n"
                << "Time[ms]: "
                << elapsed_time_ms
                << "\nNumber line: "
                << number_line
                << "\n\n"
                << std::endl;
    }

    {
      std::cout << "Start benchmark read" << std::endl;

      const auto time_start = std::chrono::high_resolution_clock::now();
      const auto number_new_line = benchmark_read();
      const auto time_end = std::chrono::high_resolution_clock::now();
      double elapsed_time_ms = std::chrono::duration<double, std::milli>(time_end - time_start).count();

      std::cout << "Benchmark read is success finish: \n"
                << "Time[ms]: "
                << elapsed_time_ms
                << "\nNumber new lines="
                << number_new_line
                << "\n\n"
                << std::endl;
    }

    {
      std::cout << "Start benchmark FileReader[string_view]" << std::endl;

      const auto time_start = std::chrono::high_resolution_clock::now();
      const auto number_line = benchmark_file_reader_stringview();
      const auto time_end = std::chrono::high_resolution_clock::now();
      double elapsed_time_ms = std::chrono::duration<double, std::milli>(time_end - time_start).count();

      std::cout << "Benchmark FileReader[string_view] is success finish: \n"
                << "Time[ms]: "
                << elapsed_time_ms
                << "\nNumber line: "
                << number_line
                << "\n\n"
                << std::endl;
    }

    {
      std::cout << "Start benchmark FileReader[string]" << std::endl;

      const auto time_start = std::chrono::high_resolution_clock::now();
      const auto number_line = benchmark_file_reader_string();
      const auto time_end = std::chrono::high_resolution_clock::now();
      double elapsed_time_ms = std::chrono::duration<double, std::milli>(time_end - time_start).count();

      std::cout << "Benchmark FileReader[string] is success finish: \n"
                << "Time[ms]: "
                << elapsed_time_ms
                << "\nNumber line: "
                << number_line
                << "\n\n"
                << std::endl;
    }

    return EXIT_SUCCESS;
  }

private:
  std::uint64_t benchmark_read()
  {
    const int file = ::open(file_path_.c_str(), O_RDONLY);
    if(file == -1)
    {
      std::ostringstream stream;
      stream << "Can't open file="
             << file_path_;
      throw std::runtime_error(stream.str());
    }

    std::vector<char> buffer;
    buffer.resize(size_buffer_);
    std::size_t size_buffer = size_buffer_ - 1;
    buffer[size_buffer] = '\n';
    char* const pBuffer = buffer.data();
    char* pEnd = pBuffer + size_buffer;

    std::uint64_t count = 0;
    char* pointer = pEnd;
    while (true)
    {
      if (*pointer != '\n') [[likely]]
      {
        pointer += 1;
      }
      else
      {
        if (pointer != pEnd) [[likely]]
        {
          pointer += 1;
          count += 1;
        }
        else
        {
          const auto result = ::read(file, pBuffer, size_buffer);
          if (result <= 0)
          {
            if (result == 0)
            {
              break;
            }
            else
            {
              throw std::runtime_error("Bad file");
            }
          }
          else if (static_cast<std::size_t>(result) < (size_buffer))
          {
            size_buffer = static_cast<std::size_t>(result);
            buffer.resize(size_buffer + 1);
            buffer[size_buffer] = '\n';
            pEnd = pBuffer + size_buffer;
          }
          pointer = pBuffer;
        }
      }
    }

    return count;
  }

  std::size_t benchmark_file_reader_string()
  {
    UServerUtils::FileManager::File file(file_path_, O_RDONLY);
    if (!file)
    {
      std::ostringstream stream;
      stream << FNS
             << "Can't open file="
             << file_path_
             << ", reason: "
             << file.error_message();
      throw std::runtime_error(stream.str());
    }

    UServerUtils::FileManager::FileReader reader(file, '\n', size_buffer_);
    std::string line;
    line.reserve(400);
    std::size_t count_line = 0;
    while (!reader.eof() && reader.peek() != std::char_traits<char>::eof())
    {
      count_line += 1;
      reader.getline(line);
    }

    if (reader.bad())
    {
      std::ostringstream stream;
      stream << FNS
             << "Bad file="
             << file_path_;
      throw std::runtime_error(stream.str());
    }

    return count_line;
  }

  std::size_t benchmark_file_reader_stringview()
  {
    UServerUtils::FileManager::File file(file_path_, O_RDONLY);
    if (!file)
    {
      std::ostringstream stream;
      stream << FNS
             << "Can't open file="
             << file_path_
             << ", reason: "
             << file.error_message();
      throw std::runtime_error(stream.str());
    }

    UServerUtils::FileManager::FileReader reader(file, '\n', size_buffer_);
    std::string_view line;
    std::size_t count_line = 0;
    while (!reader.eof() && reader.peek() != std::char_traits<char>::eof())
    {
      count_line += 1;
      reader.getline(line);
    }

    if (reader.bad())
    {
      std::ostringstream stream;
      stream << FNS
             << "Bad file="
             << file_path_;
      throw std::runtime_error(stream.str());
    }

    return count_line;
  }

  std::size_t benchmark_istream()
  {
    std::ifstream file(file_path_);
    if (!file)
    {
      std::ostringstream stream;
      stream << FNS
             << "Can't open file="
             << file_path_;
      throw std::runtime_error(stream.str());
    }

    std::string line;
    line.reserve(4096);
    std::size_t count_line = 0;
    while (!file.eof() && file.peek() != std::char_traits<char>::eof())
    {
      std::getline(file, line, '\n');
      count_line += 1;
    }

    if (file.bad() || file.fail())
    {
      std::ostringstream stream;
      stream << FNS
             << "Bad file="
             << file_path_;
      throw std::runtime_error(stream.str());
    }

    return count_line;
  }

  void create_file(
    const std::size_t number_line,
    const std::size_t length_line_min,
    const std::size_t length_line_max,
    const std::string& file_path)
  {
    std::remove(file_path.c_str());

    std::ofstream ofs(file_path, std::ios::trunc);
    if (!ofs)
    {
      std::ostringstream stream;
      stream << FNS
             << "Can't open file="
             << file_path;
      throw std::runtime_error(stream.str());
    }

    std::random_device device;
    std::mt19937 rng(device());
    std::uniform_int_distribution<std::mt19937::result_type> distribution(
      length_line_min,
      length_line_max);

    const std::size_t number_string = 1000;
    std::vector<std::string> strings;
    strings.reserve(number_string);
    for (std::size_t i = 0; i < number_string; i += 1)
    {
      strings.emplace_back(distribution(rng), 'a');
    }

    for (std::size_t i = 0; i < number_line; ++i)
    {
      ofs << strings[i % number_string] << '\n';
    }

    if (!ofs)
    {
      std::ostringstream stream;
      stream << FNS
             << "Creation file="
             << file_path
             << " is failed";
      throw std::runtime_error(stream.str());
    }
  }

private:
  const std::size_t number_line_;

  const std::string file_path_;

  const std::size_t size_buffer_;

  const std::size_t length_line_min_;

  const std::size_t length_line_max_;
};

int main(int, char**)
{
  try
  {
    const std::string file_path = "/u03/test/test_file";
    const std::size_t number_line = 100000000;
    const std::size_t length_line_min = 50;
    const std::size_t length_line_max = 150;
    const std::size_t size_buffer = 1024 * 256;

    return Application{
      number_line,
      length_line_min,
      length_line_max,
      file_path,
      size_buffer}.run();
  }
  catch (const eh::Exception& exc)
  {
    std::cerr << "Fatal error : "
              << exc.what()
              << std::endl;
  }

  return EXIT_FAILURE;
}