#include "datax.h"

#include <cstdlib>
#include <fstream>

class Implementation : public datax::DataX {
 public:
  Implementation();
  Implementation(const Implementation &) = delete;
  Implementation(Implementation &&) = delete;

  nlohmann::json Configuration() override;
  datax::RawMessage NextRaw() override;

  datax::Message Next() override {
    return datax::Message();
  }
  void Emit(const nlohmann::json &message) override {

  }
  void EmitRaw(const std::vector<unsigned char> &data) override {

  }
  void EmitRaw(const unsigned char *data, int dataSize) override {

  }

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
  message.Data.resize(size + 1);
  incoming->read(reinterpret_cast<char *>(message.Data.data()), size);
  return message;
}

std::shared_ptr<datax::DataX> datax::New() {
  return Implementation::Instance();
}