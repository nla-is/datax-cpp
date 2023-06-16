//
// Created by Giuseppe Coviello on 6/16/23.
//

#pragma once

#include "datax.h"
#include <grpcpp/grpcpp.h>

#include "queue.h"

#include "datax-sdk-protocol-v1.grpc.pb.h"

namespace datax::sdk::implementation {
int64_t now();
std::string Getenv(const std::string &variable);

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

class Implementation : public datax::DataX {
 public:
  explicit Implementation(std::initializer_list<datax::Option *> options);
  Implementation(const Implementation &) = delete;
  Implementation(Implementation &&) = delete;

  nlohmann::json Configuration() override;
  datax::RawMessage NextRaw() override;
  datax::Message Next() override;

  void Emit(const nlohmann::json &message, const std::string &reference = "") override;
  void EmitRaw(const std::vector<unsigned char> &data, const std::string &reference = "") override;
  void EmitRaw(const unsigned char *data, int dataSize, const std::string &reference = "") override;
  std::vector<nlohmann::json> FanOut(const std::vector<nlohmann::json> &messages) override;

 private:
  void report();
  std::shared_ptr<grpc::Channel> clientConn;
  std::unique_ptr<datax::sdk::protocol::v1::DataX::Stub> client;

  int64_t receivingTime;
  int64_t decodingTime;
  int64_t encodingTime;
  int64_t transferringTime;

  int64_t previousReceivingTime;
  int64_t previousDecodingTime;
  int64_t previousEncodingTime;
  int64_t previousTransferringTime;

  int64_t latestReport;

  datax::sdk::protocol::v1::Initialization initialization;

  friend WithFanOut;
  std::shared_ptr<Queue<nlohmann::json>> fanOutInputQueue;
  std::shared_ptr<Queue<nlohmann::json>> fanOutOutputQueue;
};
}
