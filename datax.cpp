#include "datax.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <utility>
#include <mutex>
#include <condition_variable>

#include "implementation.h"

std::shared_ptr<datax::DataX> datax::New() {
  return New(std::initializer_list<datax::Option>());
}

std::shared_ptr<datax::DataX> datax::New(std::initializer_list<datax::Option> options) {
  return std::make_shared<datax::sdk::implementation::Implementation>(options);
}
