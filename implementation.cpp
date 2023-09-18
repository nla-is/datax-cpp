//
// Created by Giuseppe Coviello on 6/16/23.
//

#include "implementation.h"

#include <fstream>
#include <sstream>
#include <future>

#include <inttypes.h>
#include <dlfcn.h>

using datax::sdk::implementation::Implementation;

int64_t datax::sdk::implementation::now() {
  return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string datax::sdk::implementation::Getenv(const std::string &variable) {
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
            "[DataX] Time for receiving: %" PRId64 " ms (%0.2lf%%), decoding: %" PRId64
            " ms (%0.2lf%%), processing: %" PRId64 " ms (%0.2lf%%), encoding: %" PRId64
            " ms (%0.2lf%%), transferring: %" PRId64 " ms (%0.2lf%%)\n",
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
  auto sidecar_library = Getenv("DATAX_SIDECAR_LIBRARY");
  if (sidecar_library.empty()) {
    throw Exception("sidecar library not provided");
  }

  sidecar_library_handle = dlopen(sidecar_library.c_str(), RTLD_LAZY | RTLD_LOCAL);
  if (sidecar_library_handle == nullptr) {
    std::ostringstream oss;
    oss << "error " << dlerror() << " opening sidecar library '" << sidecar_library << "'";
    throw Exception(oss.str());
  }

  datax_sdk_v3_begin_initialize = (void (*)()) dlsym(sidecar_library_handle,
                                                     "datax_sdk_v3_begin_initialize");
  if (datax_sdk_v3_begin_initialize == nullptr) {
    std::ostringstream oss;
    oss << "error " << dlerror() << " loading function 'datax_sdk_v3_begin_initialize'";
    throw Exception(oss.str());
  }

  datax_sdk_v3_end_initialize = (void (*)()) dlsym(sidecar_library_handle,
                                                   "datax_sdk_v3_end_initialize");
  if (datax_sdk_v3_end_initialize == nullptr) {
    std::ostringstream oss;
    oss << "error " << dlerror() << " loading function 'datax_sdk_v3_begin_initialize'";
    throw Exception(oss.str());
  }

  datax_sdk_v3_next = (void (*)()) dlsym(sidecar_library_handle,
                                         "datax_sdk_v3_next");
  if (datax_sdk_v3_next == nullptr) {
    std::ostringstream oss;
    oss << "error " << dlerror() << " loading function 'datax_sdk_v3_next'";
    throw Exception(oss.str());
  }

  datax_sdk_v3_get_message_reference = (const char *(*)()) dlsym(sidecar_library_handle,
                                                                 "datax_sdk_v3_get_message_reference");
  if (datax_sdk_v3_get_message_reference == nullptr) {
    std::ostringstream oss;
    oss << "error " << dlerror() << " loading function 'datax_sdk_v3_get_message_reference'";
    throw Exception(oss.str());
  }

  datax_sdk_v3_get_message_stream = (const char *(*)()) dlsym(sidecar_library_handle,
                                                              "datax_sdk_v3_get_message_stream");
  if (datax_sdk_v3_get_message_stream == nullptr) {
    std::ostringstream oss;
    oss << "error " << dlerror() << " loading function 'datax_sdk_v3_get_message_stream'";
    throw Exception(oss.str());
  }

  datax_sdk_v3_get_message_data = (const unsigned char *(*)()) dlsym(sidecar_library_handle,
                                                                     "datax_sdk_v3_get_message_data");
  if (datax_sdk_v3_get_message_data == nullptr) {
    std::ostringstream oss;
    oss << "error " << dlerror() << " loading function 'datax_sdk_v3_get_message_data'";
    throw Exception(oss.str());
  }

  datax_sdk_v3_get_message_data_size = (int32_t (*)()) dlsym(sidecar_library_handle,
                                                             "datax_sdk_v3_get_message_data_size");
  if (datax_sdk_v3_get_message_data_size == nullptr) {
    std::ostringstream oss;
    oss << "error " << dlerror() << " loading function 'datax_sdk_v3_get_message_data_size'";
    throw Exception(oss.str());
  }

  datax_sdk_v3_set_message_reference = (void (*)(const char *)) dlsym(sidecar_library_handle,
                                                                      "datax_sdk_v3_set_message_reference");
  if (datax_sdk_v3_set_message_reference == nullptr) {
    std::ostringstream oss;
    oss << "error " << dlerror() << " loading function 'datax_sdk_v3_set_message_reference'";
    throw Exception(oss.str());
  }

  datax_sdk_v3_set_message_data = (void (*)(const unsigned char *, int32_t)) dlsym(sidecar_library_handle,
                                                                                   "datax_sdk_v3_set_message_data");
  if (datax_sdk_v3_set_message_data == nullptr) {
    std::ostringstream oss;
    oss << "error " << dlerror() << " loading function 'datax_sdk_v3_set_message_data'";
    throw Exception(oss.str());
  }

  datax_sdk_v3_emit = (void (*)()) dlsym(sidecar_library_handle,
                                         "datax_sdk_v3_emit");
  if (datax_sdk_v3_emit == nullptr) {
    std::ostringstream oss;
    oss << "error " << dlerror() << " loading function 'datax_sdk_v3_emit'";
    throw Exception(oss.str());
  }

  datax_sdk_v3_begin_initialize();
  datax_sdk_v3_end_initialize();
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
  datax_sdk_v3_next();
  datax::RawMessage message;
  message.Reference = datax_sdk_v3_get_message_reference();
  message.Stream = datax_sdk_v3_get_message_stream();
  auto data = datax_sdk_v3_get_message_data();
  auto data_size = datax_sdk_v3_get_message_data_size();
  message.Data = std::vector<unsigned char>(data, data + data_size);
  report();
  receivingTime += now() - start;
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
  datax_sdk_v3_set_message_reference(reference.c_str());
  datax_sdk_v3_set_message_data(data, dataSize);
  transferringTime += now() - start;
  report();
}
