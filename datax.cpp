#include "datax.h"

#include <fstream>
#include <sstream>
#include <string_view>
#include <utility>

#include <dlfcn.h>

using datax::DataX;

std::string Getenv(const std::string &variable) {
  auto value = getenv(variable.c_str());
  if (value == nullptr) {
    return "";
  }
  return {value};
}

#define LOAD_FUNCTION(type, name) do {                 \
  name = (type) dlsym(library_handle_, #type);         \
  if (name == nullptr) {                               \
    std::ostringstream oss;                            \
    oss << dlerror() << " loading function " << #type; \
    throw datax::Exception(oss.str());                 \
  }                                                    \
} while(0)

DataX::DataX() {
  library_handle_ = dlopen("libdatax-sdk.so", RTLD_LOCAL | RTLD_NOW);
  if (library_handle_ == nullptr) {
    std::ostringstream oss;
    oss << dlerror() << " loading libdatax-sdk.so";
    throw datax::Exception(oss.str());
  }

  LOAD_FUNCTION(datax_sdk_v2_initialize, initialize_);
  LOAD_FUNCTION(datax_sdk_v2_next, next_);
  LOAD_FUNCTION(datax_sdk_v2_emit, emit_);
  LOAD_FUNCTION(datax_sdk_v2_message_stream, message_stream_);
  LOAD_FUNCTION(datax_sdk_v2_message_reference, message_reference_);
  LOAD_FUNCTION(datax_sdk_v2_message_data, message_data_);
  LOAD_FUNCTION(datax_sdk_v2_message_data_size, message_data_size_);
  LOAD_FUNCTION(datax_sdk_v2_message_close, message_close_);

  initialize_();
}

DataX::~DataX() {
}

nlohmann::json DataX::Configuration() {
  auto configurationPath = Getenv("DATAX_CONFIGURATION");
  if (configurationPath.empty()) {
    configurationPath = "/datax/configuration";
  }
  std::ifstream in(configurationPath);
  return nlohmann::json::parse(in);
}

datax::Message DataX::Next() {
  datax::Message message;
  auto msg = next_();
  message.Stream = message_stream_(msg);
  message.Reference = message_reference_(msg);
  auto data = message_data_(msg);
  auto data_size = message_data_size_(msg);
  message.Data = nlohmann::json::from_cbor(std::string_view((const char *) data, data_size), false, false);
  message_close_(msg);
  return message;
}

void DataX::Emit(const nlohmann::json &message, const std::string &reference) {
  auto data = nlohmann::json::to_cbor(message);
  emit_(data.data(), (int32_t) data.size(), reference.c_str());
}

std::shared_ptr<DataX> datax::DataX::Instance() {
  static std::shared_ptr<DataX> instance(new DataX);
  return instance;
}

std::shared_ptr<DataX> datax::New() {
  return DataX::Instance();
}

datax::Exception::Exception(std::string message) : message_(std::move(message)) {}

const char *datax::Exception::what() const noexcept {
  return message_.c_str();
}
