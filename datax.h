#ifndef _DATAX_H
#define _DATAX_H

#include <json.hpp>

namespace datax {
class Exception : public std::exception {
};

struct RawMessage {
  std::string Reference;
  std::string Stream;
  std::vector<unsigned char> Data;
};

struct Message {
  std::string Reference;
  std::string Stream;
  nlohmann::json Data;
};

class DataX {
 public:
  virtual nlohmann::json Configuration() = 0;
  virtual RawMessage NextRaw() = 0;
  virtual Message Next() = 0;
  virtual void Emit(const nlohmann::json &message, const std::string &reference = "") = 0;
  virtual void EmitRaw(const std::vector<unsigned char> &data, const std::string &reference = "") = 0;
  virtual void EmitRaw(const unsigned char *data, int dataSize, const std::string &reference = "") = 0;
  virtual std::vector<nlohmann::json> FanOut(const std::vector<nlohmann::json> &messages) = 0;
};

class FanOutHandler {
 public:
  virtual nlohmann::json Process(const nlohmann::json &message) = 0;
};

class Option {
 public:
  virtual void Apply(DataX *implementation) const = 0;
};

class WithFanOut : public Option {
 public:
  explicit WithFanOut(std::function<std::shared_ptr<FanOutHandler>()> fanOutHandlerFactory);
  void Apply(DataX *implementation) const override;

 private:
  std::function<std::shared_ptr<FanOutHandler>()> fanOutHandlerFactory;
};

std::shared_ptr<DataX> New();
std::shared_ptr<DataX> New(std::initializer_list<Option> options);
}

#endif //_DATAX_H
