// GTEST
#include <gtest/gtest.h>

// STD
#include <atomic>
#include <unordered_map>

// PROTO
#include "test_service.cobrazz.pb.hpp"
#include "test.grpc.pb.h"

// THIS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/Grpc/Server/ConfigCoro.hpp>
#include <UServerUtils/Grpc/Server/ServerBuilder.hpp>
#include <UServerUtils/ComponentsBuilder.hpp>
#include <UServerUtils/Manager.hpp>

using namespace UServerUtils;

namespace
{

const std::string kRequestOk = "Ok";
const std::string kRequestFinish = "Finish";
const std::string kRequestException = "Exception";

const grpc::StatusCode kStatusCodeFinish = grpc::StatusCode::INTERNAL;
const std::string kStatusCodeFinishErrorMessage = "error_message";
const std::string kStatusCodeFinishErrorDetails = "error_details";

const std::size_t kNumberOkRequest = 10;
const std::size_t kNumberCoroutine = 3;

std::atomic<int> kCountEvent{0};

class StreamStreamCoroSetService_Ok final
  : public test::TestService_HandlerStreamStream_Service,
    public ReferenceCounting::AtomicImpl
{
public:
  StreamStreamCoroSetService_Ok() = default;

  ~StreamStreamCoroSetService_Ok() override = default;

  void handle(const Reader& reader) override
  {
    while (true)
    {
      const auto data = reader.read();
      const auto status = data.status;
      if (status == ReadStatus::Finish)
      {
        kCountEvent.fetch_add(1);
        break;
      }
      else if (status == ReadStatus::Initialize)
      {
        kCountEvent.fetch_add(1);
        std::lock_guard lock(mutex_);
        counter_[data.id_rpc] = 1;
      }
      else if (status == ReadStatus::ReadsDone)
      {
        kCountEvent.fetch_add(1);
        auto& writer = data.writer;
        EXPECT_TRUE(writer);
        if (!writer)
          continue;

        grpc::Status status = grpc::Status::OK;
        const auto writer_status = writer->finish(std::move(status));
        EXPECT_EQ(writer_status, WriterStatus::Ok);
      }
      else if (status == ReadStatus::RpcFinish)
      {
        kCountEvent.fetch_add(1);
        std::lock_guard lock(mutex_);
        counter_.erase(data.id_rpc);
      }
      else if (status == ReadStatus::Read)
      {
        kCountEvent.fetch_add(1);
        auto& request = data.request;
        EXPECT_TRUE(request);
        if (!request)
          continue;

        auto& writer = data.writer;
        EXPECT_TRUE(writer);
        if (!writer)
          continue;

        const auto& message = request->message();
        if (message == kRequestOk)
        {
          std::string message_response = message;
          int count = 0;
          {
            std::lock_guard lock(mutex_);
            count = counter_[data.id_rpc];
            counter_[data.id_rpc] += 1;
          }
          message_response += std::to_string(count);

          auto response = std::make_unique<Response>();
          response->set_message(std::move(message_response));
          const auto writer_status = writer->write(std::move(response));
          EXPECT_EQ(writer_status, WriterStatus::Ok);
        }
      }
      else
      {
        EXPECT_TRUE(false);
      }
    }
  }

private:
  userver::engine::Mutex mutex_;

  std::unordered_map<Reader::IdRpc, int> counter_;
};

using StreamStreamCoroSetService_Ok_var = ReferenceCounting::SmartPtr<StreamStreamCoroSetService_Ok>;

class StreamStreamClient_Ok final
{
public:
  explicit StreamStreamClient_Ok(
    const std::shared_ptr<::grpc::Channel>& channel)
    : stub_(test::TestService::NewStub(channel))
  {
  }

  void request(const std::string& data)
  {
    grpc::ClientContext context;

    auto reader_writer = stub_->HandlerStreamStream(&context);
    EXPECT_TRUE(reader_writer);
    if (!reader_writer)
    {
      return;
    }

    std::size_t count = 0;
    while (true)
    {
      count += 1;

      if (count <= kNumberOkRequest)
      {
        test::Request request;
        request.set_message(data);
        const auto write_status = reader_writer->Write(request);
        EXPECT_TRUE(write_status);
        if (!write_status)
          break;

        test::Reply reply;
        const auto read_status = reader_writer->Read(&reply);
        EXPECT_TRUE(read_status);
        if (!read_status)
          break;
        EXPECT_EQ(data + std::to_string(count), reply.message());
      }
      else
      {
        const auto write_status = reader_writer->WritesDone();
        EXPECT_TRUE(write_status);
        if (!write_status)
          break;

        const auto status = reader_writer->Finish();
        EXPECT_TRUE(status.ok());
        break;
      }
    }
  }

private:
  std::unique_ptr<test::TestService::Stub> stub_;
};

class GrpcFixtureStreamStreamCoroSet_Ok : public testing::Test
{
public:
  void SetUp() override
  {
    logger_ = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL));

    CoroPoolConfig coro_pool_config;
    EventThreadPoolConfig event_thread_pool_config;
    TaskProcessorConfig main_task_processor_config;
    main_task_processor_config.name = "main_task_processor";
    main_task_processor_config.worker_threads = 3;
    main_task_processor_config.thread_name = "main_tskpr";

    auto task_processor_container_builder =
      std::make_unique<TaskProcessorContainerBuilder>(
        logger_.in(),
        coro_pool_config,
        event_thread_pool_config,
        main_task_processor_config);

    auto init_func = [logger = logger_, port = port_] (
      TaskProcessorContainer& task_processor_container) {
      auto& main_task_processor =
        task_processor_container.get_main_task_processor();

      auto components_builder =
        std::make_unique<ComponentsBuilder>();

      Grpc::Server::ConfigCoro config;
      config.num_threads = 3;
      config.port = port;
      config.max_size_queue = {};

      auto grpc_builder = std::make_unique<Grpc::Server::ServerBuilder>(
        config,
        logger);
      auto service = StreamStreamCoroSetService_Ok_var(
        new StreamStreamCoroSetService_Ok);
      grpc_builder->add_service(
        service.in(),
        main_task_processor,
        kNumberCoroutine);

      components_builder->add_grpc_cobrazz_server(
        std::move(grpc_builder));

      return components_builder;
    };

    manager_ =
      Manager_var(
        new Manager(
          std::move(task_processor_container_builder),
          std::move(init_func),
          logger_.in()));
  }

  void TearDown() override
  {
  }

  std::size_t port_ = 7778;

  Logging::Logger_var logger_;

  Manager_var manager_;
};

} // namespace

TEST_F(GrpcFixtureStreamStreamCoroSet_Ok, CoroSet_Ok)
{
  manager_->activate_object();

  auto channel =
    grpc::CreateChannel(
      "127.0.0.1:" + std::to_string(port_),
      grpc::InsecureChannelCredentials());

  const std::size_t number_cycle = 10;
  for (std::size_t i = 1; i <= number_cycle; ++i)
  {
    StreamStreamClient_Ok client(channel);
    client.request(kRequestOk);
  }

  StreamStreamClient_Ok client(channel);
  for (std::size_t i = 1; i <= number_cycle; ++i)
  {
    client.request(kRequestOk);
  }

  manager_->deactivate_object();
  manager_->wait_object();

  EXPECT_EQ(kCountEvent.exchange(0), 2 * number_cycle * (kNumberOkRequest + 3) + kNumberCoroutine);
}

namespace
{

class StreamStreamCoroSetService_Finish final
  : public test::TestService_HandlerStreamStream_Service,
    public ReferenceCounting::AtomicImpl
{
public:
  StreamStreamCoroSetService_Finish() = default;

  ~StreamStreamCoroSetService_Finish() override = default;

  void handle(const Reader& reader) override
  {
    while (true)
    {
      const auto data = reader.read();
      const auto status = data.status;
      if (status == ReadStatus::Finish)
      {
        kCountEvent.fetch_add(1);
        break;
      }
      else if (status == ReadStatus::Initialize)
      {
        kCountEvent.fetch_add(1);
      }
      else if (status == ReadStatus::ReadsDone)
      {
        kCountEvent.fetch_add(1);
      }
      else if (status == ReadStatus::RpcFinish)
      {
        kCountEvent.fetch_add(1);
      }
      else if (status == ReadStatus::Read)
      {
        kCountEvent.fetch_add(1);

        auto& request = data.request;
        EXPECT_TRUE(request);
        if (!request)
          continue;

        auto& writer = data.writer;
        EXPECT_TRUE(writer);
        if (!writer)
          continue;

        grpc::Status status(
          kStatusCodeFinish,
          kStatusCodeFinishErrorMessage,
          kStatusCodeFinishErrorDetails);
        const auto status_writer = writer->finish(std::move(status));
        EXPECT_EQ(status_writer, WriterStatus::Ok);
      }
      else
      {
        EXPECT_TRUE(false);
      }
    }
  }
};

using StreamStreamCoroSetService_Finish_var = ReferenceCounting::SmartPtr<StreamStreamCoroSetService_Finish>;

class StreamStreamClient_Finish final
{
public:
  explicit StreamStreamClient_Finish(
    const std::shared_ptr<::grpc::Channel>& channel)
    : stub_(test::TestService::NewStub(channel))
  {
  }

  void request(const std::string& data)
  {
    grpc::ClientContext context;

    auto reader_writer = stub_->HandlerStreamStream(&context);
    EXPECT_TRUE(reader_writer);
    if (!reader_writer)
    {
      return;
    }

    test::Request request;
    request.set_message(data);
    const auto write_status = reader_writer->Write(request);
    EXPECT_TRUE(write_status);
    if (!write_status)
      return;

    test::Reply reply;
    const auto read_status = reader_writer->Read(&reply);
    EXPECT_FALSE(read_status);

    const auto status = reader_writer->Finish();
    EXPECT_EQ(status.error_code(), kStatusCodeFinish);
    EXPECT_EQ(status.error_message(), kStatusCodeFinishErrorMessage);
    EXPECT_EQ(status.error_details(), kStatusCodeFinishErrorDetails);
  }

private:
  std::unique_ptr<test::TestService::Stub> stub_;
};

class GrpcFixtureStreamStreamCoroSet_Finish : public testing::Test
{
public:
  void SetUp() override
  {
    logger_ = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL));

    CoroPoolConfig coro_pool_config;
    EventThreadPoolConfig event_thread_pool_config;
    TaskProcessorConfig main_task_processor_config;
    main_task_processor_config.name = "main_task_processor";
    main_task_processor_config.worker_threads = 3;
    main_task_processor_config.thread_name = "main_tskpr";

    auto task_processor_container_builder =
      std::make_unique<TaskProcessorContainerBuilder>(
        logger_,
        coro_pool_config,
        event_thread_pool_config,
        main_task_processor_config);

    auto init_func = [logger = logger_, port = port_] (
      TaskProcessorContainer& task_processor_container) {
      auto& main_task_processor =
        task_processor_container.get_main_task_processor();

      auto components_builder =
        std::make_unique<ComponentsBuilder>();

      Grpc::Server::ConfigCoro config;
      config.num_threads = 3;
      config.port = port;
      config.max_size_queue = {};

      auto grpc_builder = std::make_unique<Grpc::Server::ServerBuilder>(
        config,
        logger);
      auto service = StreamStreamCoroSetService_Finish_var(
        new StreamStreamCoroSetService_Finish);
      grpc_builder->add_service(
        service.in(),
        main_task_processor,
        kNumberCoroutine);

      components_builder->add_grpc_cobrazz_server(
        std::move(grpc_builder));

      return components_builder;
    };

    manager_ =
      Manager_var(
        new Manager(
          std::move(task_processor_container_builder),
          std::move(init_func),
          logger_));
  }

  void TearDown() override
  {
  }

  std::size_t port_ = 7778;

  Logging::Logger_var logger_;

  Manager_var manager_;
};

} // namespace

TEST_F(GrpcFixtureStreamStreamCoroSet_Finish, CoroSet_Finish)
{
  manager_->activate_object();

  auto channel =
    grpc::CreateChannel(
      "127.0.0.1:" + std::to_string(port_),
      grpc::InsecureChannelCredentials());

  const std::size_t number_cycle = 10;
  for (std::size_t i = 1; i <= number_cycle; ++i)
  {
    StreamStreamClient_Finish client(channel);
    client.request(kRequestFinish);
  }

  manager_->deactivate_object();
  manager_->wait_object();

  EXPECT_EQ(kCountEvent.exchange(0), number_cycle * 4 + kNumberCoroutine);
}

namespace
{

const std::size_t kNumberExceptionRequest = 10;

class StreamStreamCoroSetService_Exception final
  : public test::TestService_HandlerStreamStream_Service,
    public ReferenceCounting::AtomicImpl
{
public:
  StreamStreamCoroSetService_Exception() = default;

  ~StreamStreamCoroSetService_Exception() override = default;

  void handle(const Reader& reader) override
  {
    while (true)
    {
      const auto data = reader.read();
      const auto status = data.status;
      if (status == ReadStatus::Finish)
      {
        kCountEvent.fetch_add(1);
        break;
      }
      else if (status == ReadStatus::Initialize)
      {
        kCountEvent.fetch_add(1);
      }
      else if (status == ReadStatus::ReadsDone)
      {
        kCountEvent.fetch_add(1);

        auto& writer = data.writer;
        EXPECT_TRUE(writer);
        if (!writer)
          return;

        grpc::Status status = grpc::Status::OK;
        const auto status_write = writer->finish(std::move(status));
        EXPECT_EQ(status_write, WriterStatus::Ok);
      }
      else if (status == ReadStatus::RpcFinish)
      {
        kCountEvent.fetch_add(1);
      }
      else if (status == ReadStatus::Read)
      {
        kCountEvent.fetch_add(1);
        throw std::runtime_error("test exception");
      }
      else
      {
        EXPECT_TRUE(false);
      }
    }
  }
};

using StreamStreamCoroSetService_Exception_var =
  ReferenceCounting::SmartPtr<StreamStreamCoroSetService_Exception>;

class StreamStreamClient_Exception final
{
public:
  explicit StreamStreamClient_Exception(
    const std::shared_ptr<::grpc::Channel>& channel)
    : stub_(test::TestService::NewStub(channel))
  {
  }

  void request(const std::string& data)
  {
    grpc::ClientContext context;

    auto reader_writer = stub_->HandlerStreamStream(&context);
    EXPECT_TRUE(reader_writer);
    if (!reader_writer)
    {
      return;
    }

    for (std::size_t i = 1; i <= kNumberExceptionRequest; ++i)
    {
      test::Request request;
      request.set_message(data);
      const auto write_status = reader_writer->Write(request);
      EXPECT_TRUE(write_status);
      if (!write_status)
        return;
    }

    const auto write_status = reader_writer->WritesDone();
    EXPECT_TRUE(write_status);

    test::Reply reply;
    const auto read_status = reader_writer->Read(&reply);
    EXPECT_FALSE(read_status);

    const auto status = reader_writer->Finish();
    EXPECT_EQ(status.error_code(), grpc::StatusCode::OK);
  }

private:
  std::unique_ptr<test::TestService::Stub> stub_;
};

class GrpcFixtureStreamStreamCoroSet_Exception : public testing::Test
{
public:
  void SetUp() override
  {
    logger_ = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL));

    CoroPoolConfig coro_pool_config;
    EventThreadPoolConfig event_thread_pool_config;
    TaskProcessorConfig main_task_processor_config;
    main_task_processor_config.name = "main_task_processor";
    main_task_processor_config.worker_threads = 3;
    main_task_processor_config.thread_name = "main_tskpr";

    auto task_processor_container_builder =
      std::make_unique<TaskProcessorContainerBuilder>(
        logger_,
        coro_pool_config,
        event_thread_pool_config,
        main_task_processor_config);

    auto init_func = [logger = logger_, port = port_] (
      TaskProcessorContainer& task_processor_container) {
      auto& main_task_processor =
        task_processor_container.get_main_task_processor();

      auto components_builder =
        std::make_unique<ComponentsBuilder>();

      Grpc::Server::ConfigCoro config;
      config.num_threads = 3;
      config.port = port;
      config.max_size_queue = {};

      auto grpc_builder = std::make_unique<Grpc::Server::ServerBuilder>(
        config,
        logger);
      auto service =
        StreamStreamCoroSetService_Exception_var(
          new StreamStreamCoroSetService_Exception);
      grpc_builder->add_service(
        service.in(),
        main_task_processor,
        kNumberCoroutine);

      components_builder->add_grpc_cobrazz_server(
        std::move(grpc_builder));

      return components_builder;
    };

    manager_ =
      Manager_var(
        new Manager(
          std::move(task_processor_container_builder),
          std::move(init_func),
          logger_));
  }

  void TearDown() override
  {
  }

  std::size_t port_ = 7778;

  Logging::Logger_var logger_;

  Manager_var manager_;
};

} // namespace

TEST_F(GrpcFixtureStreamStreamCoroSet_Exception, CoroSet_Exception)
{
  manager_->activate_object();

  auto channel =
    grpc::CreateChannel(
      "127.0.0.1:" + std::to_string(port_),
      grpc::InsecureChannelCredentials());

  const std::size_t number_cycle = 10;
  for (std::size_t i = 1; i <= number_cycle; ++i)
  {
    StreamStreamClient_Exception client(channel);
    client.request(kRequestFinish);
  }

  manager_->deactivate_object();
  manager_->wait_object();

  EXPECT_EQ(kCountEvent.exchange(0), number_cycle * (3 + kNumberExceptionRequest) + kNumberCoroutine);
}

namespace
{

class StreamStreamCoroPerRpc_Ok final
  : public test::TestService_HandlerStreamStream_Service,
    public ReferenceCounting::AtomicImpl
{
public:
  StreamStreamCoroPerRpc_Ok() = default;

  ~StreamStreamCoroPerRpc_Ok() override = default;

  void handle(const Reader& reader) override
  {
    std::size_t counter = 0;
    while (true)
    {
      const auto data = reader.read();
      const auto status = data.status;
      if (status == ReadStatus::Finish)
      {
        kCountEvent.fetch_add(1);
        break;
      }
      else if (status == ReadStatus::Initialize)
      {
        kCountEvent.fetch_add(1);
      }
      else if (status == ReadStatus::ReadsDone)
      {
        kCountEvent.fetch_add(1);
        auto& writer = data.writer;
        EXPECT_TRUE(writer);
        if (!writer)
          continue;

        grpc::Status status = grpc::Status::OK;
        const auto writer_status = writer->finish(std::move(status));
        EXPECT_EQ(writer_status, WriterStatus::Ok);
      }
      else if (status == ReadStatus::RpcFinish)
      {
        kCountEvent.fetch_add(1);
      }
      else if (status == ReadStatus::Read)
      {
        kCountEvent.fetch_add(1);
        auto& request = data.request;
        EXPECT_TRUE(request);
        if (!request)
          continue;

        auto& writer = data.writer;
        EXPECT_TRUE(writer);
        if (!writer)
          continue;

        const auto& message = request->message();
        if (message == kRequestOk)
        {
          std::string message_response = message + std::to_string(++counter);
          auto response = std::make_unique<Response>();
          response->set_message(std::move(message_response));
          const auto writer_status = writer->write(std::move(response));
          EXPECT_EQ(writer_status, WriterStatus::Ok);
        }
      }
      else
      {
        EXPECT_TRUE(false);
      }
    }
  }
};

using StreamStreamCoroPerRpc_Ok_var = ReferenceCounting::SmartPtr<StreamStreamCoroPerRpc_Ok>;

class GrpcFixtureStreamStreamCoroPerRpc_Ok : public testing::Test
{
public:
  void SetUp() override
  {
    logger_ = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL));

    CoroPoolConfig coro_pool_config;
    EventThreadPoolConfig event_thread_pool_config;
    TaskProcessorConfig main_task_processor_config;
    main_task_processor_config.name = "main_task_processor";
    main_task_processor_config.worker_threads = 3;
    main_task_processor_config.thread_name = "main_tskpr";

    auto task_processor_container_builder =
      std::make_unique<TaskProcessorContainerBuilder>(
        logger_,
        coro_pool_config,
        event_thread_pool_config,
        main_task_processor_config);

    auto init_func = [logger = logger_, port = port_] (
      TaskProcessorContainer& task_processor_container) {
      auto& main_task_processor =
        task_processor_container.get_main_task_processor();

      auto components_builder =
        std::make_unique<ComponentsBuilder>();

      Grpc::Server::ConfigCoro config;
      config.num_threads = 3;
      config.port = port;
      config.max_size_queue = {};

      auto grpc_builder = std::make_unique<Grpc::Server::ServerBuilder>(
        config,
        logger);
      auto service =
        StreamStreamCoroPerRpc_Ok_var(
          new StreamStreamCoroPerRpc_Ok);
      grpc_builder->add_service(
        service.in(),
        main_task_processor);

      components_builder->add_grpc_cobrazz_server(
        std::move(grpc_builder));

      return components_builder;
    };

    manager_ =
      Manager_var(
        new Manager(
          std::move(task_processor_container_builder),
          std::move(init_func),
          logger_));
  }

  void TearDown() override
  {
  }

  std::size_t port_ = 7778;

  Logging::Logger_var logger_;

  Manager_var manager_;
};

} // namespace

TEST_F(GrpcFixtureStreamStreamCoroPerRpc_Ok, CoroPerRpc_Ok)
{
  manager_->activate_object();

  auto channel =
    grpc::CreateChannel(
      "127.0.0.1:" + std::to_string(port_),
      grpc::InsecureChannelCredentials());

  const std::size_t number_cycle = 10;
  for (std::size_t i = 1; i <= number_cycle; ++i)
  {
    StreamStreamClient_Ok client(channel);
    client.request(kRequestOk);
  }

  StreamStreamClient_Ok client(channel);
  for (std::size_t i = 1; i <= number_cycle; ++i)
  {
    client.request(kRequestOk);
  }

  manager_->deactivate_object();
  manager_->wait_object();

  EXPECT_EQ(kCountEvent.exchange(0), 2 * number_cycle * (kNumberOkRequest + 4));
}