#include "datax.h"

#include <cstdlib>
#include <fstream>

class Implementation : public datax::DataX {
 public:
  Implementation();
  Implementation(const Implementation &) = delete;
  Implementation(Implementation &&) = delete;

  nlohmann::json Configuration() override;
  datax::RawMessage NextRaw() override {
    return datax::RawMessage();
  }
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

std::shared_ptr<datax::DataX> datax::New() {
  return Implementation::Instance();
}