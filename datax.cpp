#include "datax.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

#include <grpcpp/grpcpp.h>

#include "datax-sdk-protocol-v1.grpc.pb.h"

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
  std::shared_ptr<grpc::Channel> clientConn;
  std::unique_ptr<datax::sdk::protocol::v1::DataX::Stub> client;
};

std::string Getenv(const std::string &variable) {
  auto value = getenv(variable.c_str());
  if (value == nullptr) {
    return "";
  }
  return {value};
}

Implementation::Implementation() {
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
  return message;
}

datax::Message Implementation::Next() {
  auto raw = NextRaw();
  datax::Message msg;
  msg.Stream = raw.Stream;
  msg.Reference = raw.Reference;
  printf("Data size: %lu", raw.Data.size());
  msg.Data = nlohmann::json::from_msgpack(raw.Data);
  return msg;
}

void Implementation::Emit(const nlohmann::json &message, const std::string &reference) {
  EmitRaw(nlohmann::json::to_msgpack(message), reference);
}

void Implementation::EmitRaw(const std::vector<unsigned char> &data, const std::string &reference) {
  EmitRaw(data.data(), static_cast<int>(data.size()), reference);
}

void Implementation::EmitRaw(const unsigned char *data, int dataSize, const std::string &reference) {
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
}

std::shared_ptr<datax::DataX> datax::New() {
  return Implementation::Instance();
}
