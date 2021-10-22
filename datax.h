#ifndef _DATAX_H
#define _DATAX_H

#include <json.hpp>

namespace datax {
class Exception : std::exception {
};

struct RawMessage {
  std::string Stream;
  std::vector<unsigned char> Data;
};

struct Message {
  std::string Stream;
  nlohmann::json Data;
};

class DataX {
 public:
  virtual nlohmann::json Configuration() = 0;
  virtual RawMessage NextRaw() = 0;
  virtual Message Next() = 0;
  virtual void Emit(const nlohmann::json &message) = 0;
  virtual void EmitRaw(const std::vector<unsigned char> &data) = 0;
  virtual void EmitRaw(const unsigned char *data, int dataSize) = 0;
};

std::shared_ptr<DataX> New();
}

#endif //_DATAX_H
