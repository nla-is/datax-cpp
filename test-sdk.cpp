//
// Created by Giuseppe Coviello on 10/22/21.
//

#include "datax.h"

#include <iostream>

int main() {
  auto dx = datax::New();

  std::cout << "Configuration: " << dx->Configuration() << std::endl;

  auto raw = dx->NextRaw();
  std::cout << "Received " << raw.Data.size() << " byte(s) from '" << raw.Stream << "'" << std::endl;

  auto msg = dx->Next();
  std::cout << "Received " << msg.Data << " byte(s) from '" << msg.Stream << "'" << std::endl;
}