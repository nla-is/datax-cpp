#pragma once

#include <json.hpp>

namespace datax {
class Exception : public std::exception {
 public:
  explicit Exception(std::string message);
  const char *what() const noexcept override;

 private:
  std::string message_;
};

struct Message {
  std::string Reference;
  std::string Stream;
  nlohmann::json Data;
};

class DataX {
 public:
  static std::shared_ptr<DataX> Instance();

  DataX(const DataX &) = delete;
  virtual ~DataX();

  static nlohmann::json Configuration();
  Message Next();
  void Emit(const nlohmann::json &message, const std::string &reference = "");

 private:
  DataX();
  typedef uintptr_t datax_sdk_v2_message;
  typedef void (*datax_sdk_v2_initialize)();
  typedef datax_sdk_v2_message (*datax_sdk_v2_next)();
  typedef void (*datax_sdk_v2_emit)(const unsigned char * data, int32_t data_size, const char * reference);
  typedef void (*datax_sdk_v2_message_close)(datax_sdk_v2_message message);
  typedef const char * (*datax_sdk_v2_message_reference)(datax_sdk_v2_message message);
  typedef const char * (*datax_sdk_v2_message_stream)(datax_sdk_v2_message message);
  typedef const unsigned char * (*datax_sdk_v2_message_data)(datax_sdk_v2_message message);
  typedef int32_t (*datax_sdk_v2_message_data_size)(datax_sdk_v2_message message);

  void *library_handle_;
  datax_sdk_v2_initialize initialize_;
  datax_sdk_v2_next next_;
  datax_sdk_v2_emit emit_;
  datax_sdk_v2_message_stream message_stream_;
  datax_sdk_v2_message_reference message_reference_;
  datax_sdk_v2_message_data message_data_;
  datax_sdk_v2_message_data_size message_data_size_;
  datax_sdk_v2_message_close message_close_;
};

std::shared_ptr<DataX> New();
}
