#include "datax.h"

class Implementation : public datax::DataX {
 public:
  nlohmann::json Configuration() override {
    return nlohmann::json();
  }
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

std::shared_ptr<datax::DataX> datax::New() {
  return Implementation::Instance();
}