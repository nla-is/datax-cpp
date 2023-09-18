#include "datax.h"

#include "implementation.h"

std::shared_ptr<datax::DataX> datax::New() {
  return std::make_shared<datax::sdk::implementation::Implementation>();
}
