//
// Created by Giuseppe Coviello on 6/16/23.
//

#include <thread>
#include <utility>

#include "datax.h"
#include "implementation.h"

namespace datax {
WithFanOut::WithFanOut(std::function<std::shared_ptr<FanOutHandler>()> fanOutHandlerFactory) :
fanOutHandlerFactory(std::move(fanOutHandlerFactory)) {}

void WithFanOut::Apply(DataX *_implementation) const {
  auto implementation = reinterpret_cast<datax::sdk::implementation::Implementation *>(_implementation);
  implementation->fanOutInputQueue = std::make_shared<datax::sdk::implementation::Queue<nlohmann::json>>();
  implementation->fanOutOutputQueue = std::make_shared<datax::sdk::implementation::Queue<nlohmann::json>>();
  auto number_of_handlers = 1;
  if (implementation->initialization.with_fan_out()) {
    number_of_handlers = 5;
  }

  for (auto i = 0; i < number_of_handlers; i++) {
    std::thread t([&](){
      auto handler = fanOutHandlerFactory();
      while (true) {
        auto msg = implementation->fanOutInputQueue->pop();
        implementation->fanOutOutputQueue->push(handler->Process(msg));
      }
    });
    t.detach();
  }
}
}