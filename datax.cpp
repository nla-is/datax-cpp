#include "datax.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

#include <grpcpp/grpcpp.h>

#include "datax-sdk-protocol-v1.grpc.pb.h"

uint64_t now() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

class Exception : public datax::Exception {
 public:
  explicit Exception(std::string message) : message(std::move(message)) {}

  const char *what() const noexcept override {
    return message.c_str();
  }

 private:
  std::string message;
};

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

  uint64_t receivingTime;
  uint64_t decodingTime;
  uint64_t encodingTime;
  uint64_t transferringTime;

  uint64_t previousReceivingTime;
  uint64_t previousDecodingTime;
  uint64_t previousEncodingTime;
  uint64_t previousTransferringTime;

  uint64_t latestReport;
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
            "[DataX] Time for receiving: %llu ms (%0.2f%%), decoding: %llu ms (%0.2f%%), processing: %llu ms (%0.2f%%), encoding: %llu ms (%0.2f%%), transferring: %llu ms (%0.2f%%)\n",
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

  previousReceivingTime = receivingTime;
  previousDecodingTime = decodingTime;
  previousEncodingTime = encodingTime;
  previousTransferringTime = transferringTime;

  latestReport = now();
}

Implementation::Implementation() : receivingTime(0), decodingTime(0),
                                   encodingTime(0), transferringTime(0),
                                   latestReport(0) {
  auto sidecarAddress = Getenv("DATAX_SIDECAR_ADDRESS");
  if (sidecarAddress.empty()) {
    sidecarAddress = "127.0.0.1:20001";
  }
  clientConn = grpc::CreateChannel(sidecarAddress, grpc::InsecureChannelCredentials());
  client = datax::sdk::protocol::v1::DataX::NewStub(clientConn);
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
  const auto &data = reply.data();
  if (data.empty()) {
    fprintf(stderr, "Empty data\n");
    exit(-1);
  }
  message.Data = std::vector<unsigned char>(reinterpret_cast<const unsigned char *>(data.data()),
                                            reinterpret_cast<const unsigned char *>(data.data()) + data.size());
  message.Reference = reply.reference();
  message.Stream = reply.stream();
  receivingTime = now() - start;
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
  decodingTime = now() - start;
  return msg;
}

void Implementation::Emit(const nlohmann::json &message, const std::string &reference) {
  auto start = now();
  auto data = nlohmann::json::to_msgpack(message);
  encodingTime = now() - start;
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

  request.set_data(data, dataSize);
  request.set_reference(reference);

  auto status = client->Emit(&context, request, &reply);
  if (!status.ok()) {
    std::ostringstream oss;
    oss << status.error_code() << ": " << status.error_message();
    throw Exception(oss.str());
  }
  transferringTime = now() - start;
  report();
}

std::shared_ptr<datax::DataX> datax::New() {
  return Implementation::Instance();
}
