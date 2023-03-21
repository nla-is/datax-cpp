#include "datax.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <utility>
#include <mutex>
#include <condition_variable>

#include <grpcpp/grpcpp.h>

#include "datax-sdk-protocol-v1.grpc.pb.h"

namespace datax::sdk::implementation {
int64_t now() {
  return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count());
}

class Exception : public datax::Exception {
 public:
  Exception() = default;
  explicit Exception(std::string message) : message(std::move(message)) {}

  const char *what() const noexcept override {
    return message.c_str();
  }

 private:
  std::string message;
};

class NextChannel {
 public:
  explicit NextChannel(const std::string &path);
  ~NextChannel();
  void Read(std::vector<unsigned char> *data);

 private:
  std::string path;
  std::ifstream is;
};

NextChannel::NextChannel(const std::string &path) : path(path) {
}

NextChannel::~NextChannel() {
  if (is.is_open()) {
    is.close();
  }
}

void NextChannel::Read(std::vector<unsigned char> *data) {
  if (!is.is_open()) {
    is.open(path);
  }
  is.read(reinterpret_cast<char *>(data->data()), data->size());
  if (is.fail()) {
    throw Exception("reading from next channel");
  }
}

class EmitChannel {
 public:
  explicit EmitChannel(std::string path);
  ~EmitChannel();
  void Write(std::vector<unsigned char> data);

 private:
  void run1();
  static void run(EmitChannel *ec);

  std::string path;
  std::thread runner;
  std::mutex mutex;
  std::condition_variable cv;

  bool slotEmpty;
  std::vector<unsigned char> data;

  Exception exception;
  bool throwException;
};

EmitChannel::EmitChannel(std::string path) : path(std::move(path)),
                                             runner(std::thread(EmitChannel::run, this)),
                                             throwException(false),
                                             slotEmpty(true) {
}

EmitChannel::~EmitChannel() {
  runner.join();
}

void EmitChannel::Write(std::vector<unsigned char> data) {
  if (throwException) {
    throw exception;
  }
  std::unique_lock lock(mutex);
  while (!slotEmpty) {
    cv.wait(lock);
  }
  this->data = std::move(data);
  slotEmpty = false;
  cv.notify_all();
}

void EmitChannel::run(EmitChannel *ec) {
  ec->run1();
}

void EmitChannel::run1() {
  std::ofstream os(path);
  if (!os.is_open()) {
    exception = Exception("opening emit channel");
    throwException = true;
    return;
  }

  while (true) {
    std::vector<unsigned char> data;
    {
      std::unique_lock lock(mutex);
      while (slotEmpty) {
        cv.wait(lock);
      }
      data = std::move(this->data);
      slotEmpty = true;
    }
    cv.notify_all();

    os.write(reinterpret_cast<const char *>(data.data()), data.size()).flush();
    if (!os.good()) {
      exception = Exception("writing to emit channel");
      throwException = true;
      return;
    }
  }
}

class Implementation : public datax::DataX {
 public:
  Implementation();
  Implementation(const Implementation &) = delete;
  Implementation(Implementation &&) = delete;

  nlohmann::json Configuration() override;
  datax::RawMessage NextRaw() override;
  datax::Message Next() override;

  void Emit(const nlohmann::json &message, const std::string &reference = "") override;
  void EmitRaw(const std::vector<unsigned char> &data, const std::string &reference = "") override;
  void EmitRaw(const unsigned char *data, int dataSize, const std::string &reference = "") override;

  static std::shared_ptr<datax::DataX> Instance() {
    std::shared_ptr<datax::DataX> instance(new Implementation);
    return instance;
  }

 private:
  void report();
  std::shared_ptr<grpc::Channel> clientConn;
  std::unique_ptr<datax::sdk::protocol::v1::DataX::Stub> client;

  std::unique_ptr<NextChannel> nextChannel;
  std::unique_ptr<EmitChannel> emitChannel;

  int64_t receivingTime;
  int64_t decodingTime;
  int64_t encodingTime;
  int64_t transferringTime;

  int64_t previousReceivingTime;
  int64_t previousDecodingTime;
  int64_t previousEncodingTime;
  int64_t previousTransferringTime;

  int64_t latestReport;
};

std::string Getenv(const std::string &variable) {
  auto value = getenv(variable.c_str());
  if (value == nullptr) {
    return "";
  }
  return {value};
}

void Implementation::report() {
  if (now() - latestReport < 10000) {
    return;
  }
  if (latestReport > 0) {
    auto elapsedTime = now() - latestReport;

    auto receiving = receivingTime - previousReceivingTime;
    auto decoding = decodingTime - previousDecodingTime;
    auto encoding = encodingTime - previousEncodingTime;
    auto transferring = transferringTime - previousTransferringTime;

    auto processing = elapsedTime - receiving - decoding - encoding - transferring;
    fprintf(stderr,
            "[DataX] Time for receiving: %ld ms (%0.2lf%%), decoding: %ld ms (%0.2lf%%), processing: %ld ms (%0.2lf%%), encoding: %ld ms (%0.2lf%%), transferring: %ld ms (%0.2lf%%)\n",
            receiving,
            (((double) receiving) / ((double) elapsedTime)) * 100,
            decoding,
            (((double) decoding) / ((double) elapsedTime)) * 100,
            processing,
            (((double) processing) / ((double) elapsedTime)) * 100,
            encoding,
            (((double) encoding) / ((double) elapsedTime)) * 100,
            transferring,
            (((double) transferring) / ((double) elapsedTime)) * 100
    );
  }

  assert(receivingTime >= previousReceivingTime);
  previousReceivingTime = receivingTime;
  assert(decodingTime >= previousDecodingTime);
  previousDecodingTime = decodingTime;
  assert(encodingTime >= previousEncodingTime);
  previousEncodingTime = encodingTime;
  assert(transferringTime >= previousTransferringTime);
  previousTransferringTime = transferringTime;

  latestReport = now();
}

Implementation::Implementation() : receivingTime(0), decodingTime(0),
                                   encodingTime(0), transferringTime(0),
                                   previousReceivingTime(0), previousDecodingTime(0),
                                   previousEncodingTime(0), previousTransferringTime(0),
                                   latestReport(0) {
  auto sidecarAddress = Getenv("DATAX_SIDECAR_ADDRESS");
  if (sidecarAddress.empty()) {
    sidecarAddress = "127.0.0.1:20001";
  }
  clientConn = grpc::CreateChannel(sidecarAddress, grpc::InsecureChannelCredentials());
  client = datax::sdk::protocol::v1::DataX::NewStub(clientConn);
  grpc::ClientContext context;
  datax::sdk::protocol::v1::Settings settings;
  settings.set_optimizeddatachannel(true);
  datax::sdk::protocol::v1::Initialization initialization;
  auto status = client->Initialize(&context, settings, &initialization);
  if (!status.ok()) {
    std::ostringstream oss;
    oss << status.error_code() << ": " << status.error_message();
    throw Exception(oss.str());
  }
  if (!initialization.nextchannelpath().empty()) {
    nextChannel = std::make_unique<NextChannel>(initialization.nextchannelpath());
  }
  if (!initialization.emitchannelpath().empty()) {
    emitChannel = std::make_unique<EmitChannel>(initialization.emitchannelpath());
  }
}

nlohmann::json Implementation::Configuration() {
  auto configurationPath = Getenv("DATAX_CONFIGURATION");
  if (configurationPath.empty()) {
    configurationPath = "/datax/configuration";
  }
  std::ifstream in(configurationPath);
  return nlohmann::json::parse(in);
}

datax::RawMessage Implementation::NextRaw() {
  auto start = now();
  grpc::ClientContext context;
  datax::sdk::protocol::v1::NextOptions request;
  datax::sdk::protocol::v1::NextMessage reply;
  auto status = client->Next(&context, request, &reply);
  if (!status.ok()) {
    std::ostringstream oss;
    oss << status.error_code() << ": " << status.error_message();
    throw Exception(oss.str());
  }
  datax::RawMessage message;

  if (nextChannel) {
    message.Data.resize(reply.datasize());
    nextChannel->Read(&message.Data);
  } else {
    const auto &data = reply.data();
    if (data.empty()) {
      fprintf(stderr, "Empty data\n");
      exit(-1);
    }
    message.Data = std::vector<unsigned char>(reinterpret_cast<const unsigned char *>(data.data()),
                                              reinterpret_cast<const unsigned char *>(data.data()) + data.size());
  }

  message.Reference = reply.reference();
  message.Stream = reply.stream();
  receivingTime += now() - start;
  report();
  return message;
}

datax::Message Implementation::Next() {
  auto raw = NextRaw();
  auto start = now();
  datax::Message msg;
  msg.Stream = raw.Stream;
  msg.Reference = raw.Reference;
  msg.Data = nlohmann::json::from_msgpack(raw.Data);
  decodingTime += now() - start;
  return msg;
}

void Implementation::Emit(const nlohmann::json &message, const std::string &reference) {
  auto start = now();
  auto data = nlohmann::json::to_msgpack(message);
  encodingTime += now() - start;
  EmitRaw(data, reference);
}

void Implementation::EmitRaw(const std::vector<unsigned char> &data, const std::string &reference) {
  EmitRaw(data.data(), static_cast<int>(data.size()), reference);
}

void Implementation::EmitRaw(const unsigned char *data, int dataSize, const std::string &reference) {
  auto start = now();
  grpc::ClientContext context;
  datax::sdk::protocol::v1::EmitMessage request;
  datax::sdk::protocol::v1::EmitResult reply;

  if (emitChannel) {
    request.set_datasize(dataSize);
    emitChannel->Write(std::vector<unsigned char>(data, data+dataSize));
  } else {
    request.set_data(data, dataSize);
  }
  request.set_reference(reference);

  auto status = client->Emit(&context, request, &reply);
  if (!status.ok()) {
    std::ostringstream oss;
    oss << status.error_code() << ": " << status.error_message();
    throw Exception(oss.str());
  }
  transferringTime += now() - start;
  report();
}

}

std::shared_ptr<datax::DataX> datax::New() {
  return sdk::implementation::Implementation::Instance();
}
