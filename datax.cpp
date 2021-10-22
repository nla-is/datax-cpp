#include "datax.h"

#include <cstdlib>
#include <fstream>

class Implementation : public datax::DataX {
 public:
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
};

std::string Getenv(const std::string &variable) {
  auto value = getenv(variable.c_str());
  if (value == nullptr) {
    return "";
  }
  return {value};
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