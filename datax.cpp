#include "datax.h"

#include <cstdlib>
#include <fstream>
#include <iostream>

class Implementation : public datax::DataX {
 public:
  Implementation();
  Implementation(const Implementation &) = delete;
  Implementation(Implementation &&) = delete;

  nlohmann::json Configuration() override;
  datax::RawMessage NextRaw() override;
  datax::Message Next() override;

  void Emit(const nlohmann::json &message) override;
  void EmitRaw(const std::vector<unsigned char> &data) override;
  void EmitRaw(const unsigned char *data, int dataSize) override;

  static std::shared_ptr<datax::DataX> Instance() {
    std::shared_ptr<datax::DataX> instance(new Implementation);
    return instance;
  }

 private:
  std::shared_ptr<std::ifstream> incoming;
  std::shared_ptr<std::ofstream> outgoing;
};

std::string Getenv(const std::string &variable) {
  auto value = getenv(variable.c_str());
  if (value == nullptr) {
    return "";
  }
  return {value};
}

Implementation::Implementation() {
  auto incomingPath = Getenv("DATAX_INCOMING");
  if (incomingPath.empty()) {
    incomingPath = "/datax/incoming";
  }
  auto outgoingPath = Getenv("DATAX_OUTGOING");
  if (outgoingPath.empty()) {
    outgoingPath = "/datax/outgoing";
  }
  incoming = std::make_shared<std::ifstream>(incomingPath);
  outgoing = std::make_shared<std::ofstream>(outgoingPath);
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
  int32_t size;
  incoming->read(reinterpret_cast<char *>(&size), sizeof size);
  datax::RawMessage message;
  message.Data.resize(size + 1);
  incoming->read(reinterpret_cast<char *>(message.Data.data()), size);
  message.Data[size] = 0;
  message.Stream = std::string(reinterpret_cast<char *>(message.Data.data()));

  incoming->read(reinterpret_cast<char *>(&size), sizeof size);
  message.Data.resize(size);
  incoming->read(reinterpret_cast<char *>(message.Data.data()), size);
  return message;
}

datax::Message Implementation::Next() {
  auto raw = NextRaw();
  datax::Message msg;
  msg.Stream = raw.Stream;
  msg.Data = nlohmann::json::from_msgpack(raw.Data);
  return msg;
}

void Implementation::Emit(const nlohmann::json &message) {
  EmitRaw(nlohmann::json::to_msgpack(message));
}

void Implementation::EmitRaw(const std::vector<unsigned char> &data) {
  EmitRaw(data.data(), static_cast<int>(data.size()));
}

void Implementation::EmitRaw(const unsigned char *data, int dataSize) {
  auto size = static_cast<int32_t>(dataSize);
  outgoing->write(reinterpret_cast<const char *>(&size), sizeof size);
  outgoing->flush();
  outgoing->write(reinterpret_cast<const char *>(data), size);
  outgoing->flush();
}

std::shared_ptr<datax::DataX> datax::New() {
  return Implementation::Instance();
}