#pragma once

#include <json.hpp>

namespace datax {
class Exception : public std::exception {
};

struct Message {
  std::string Reference;
  std::string Stream;
  nlohmann::json Data;
};

class DataX {
 public:
  nlohmann::json Configuration() = 0;
  Message Next() = 0;
  void Emit(const nlohmann::json &message, const std::string &reference = "") = 0;
};

std::shared_ptr<DataX> New();
}
