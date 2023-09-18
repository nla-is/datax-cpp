//
// Created by Giuseppe Coviello on 6/16/23.
//

#pragma once

#include "datax.h"

namespace datax::sdk::implementation {
int64_t now();
std::string Getenv(const std::string &variable);

class Exception : public datax::Exception {
 public:
  Exception() = default;
  explicit Exception(std::string message) : message(std::move(message)) {}

  [[nodiscard]] const char *what() const noexcept override {
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

  void Emit(const nlohmann::json &message, const std::string &reference) override;
  void EmitRaw(const std::vector<unsigned char> &data, const std::string &reference) override;
  void EmitRaw(const unsigned char *data, int dataSize, const std::string &reference) override;

 private:
  void report();

  int64_t receivingTime;
  int64_t decodingTime;
  int64_t encodingTime;
  int64_t transferringTime;

  int64_t previousReceivingTime;
  int64_t previousDecodingTime;
  int64_t previousEncodingTime;
  int64_t previousTransferringTime;

  int64_t latestReport;

  void *sidecar_library_handle;
  void (*datax_sdk_v3_begin_initialize)();
  void (*datax_sdk_v3_end_initialize)();
  void (*datax_sdk_v3_next)();
  const char *(*datax_sdk_v3_get_message_reference)();
  const char *(*datax_sdk_v3_get_message_stream)();
  const unsigned char *(*datax_sdk_v3_get_message_data)();
  int32_t (*datax_sdk_v3_get_message_data_size)();
  void (*datax_sdk_v3_set_message_reference)(const char *reference);
  void (*datax_sdk_v3_set_message_data)(const unsigned char *data, int32_t data_size);
  void (*datax_sdk_v3_emit)();
};
}
