//
// Created by Giuseppe Coviello on 6/16/23.
//

#include <thread>
#include <utility>

#include "datax.h"
#include "implementation.h"

using datax::sdk::implementation::Queue;

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
    fprintf(stderr, "[DataX] Creating fan-out handler %d\n", i);
    auto handler = fanOutHandlerFactory();
    fprintf(stderr, "[DataX] Created fan-out handler %d\n", i);
    std::thread t([](int i, const std::shared_ptr<FanOutHandler> &handler,
                     const std::shared_ptr<Queue<nlohmann::json>> &inputQueue,
                     const std::shared_ptr<Queue<nlohmann::json>> &outputQueue) {
      while (true) {
        auto msg = inputQueue->pop();
        outputQueue->push(handler->Process(msg));
      }
    }, i, handler, implementation->fanOutInputQueue, implementation->fanOutOutputQueue);
    t.detach();
  }
}
}