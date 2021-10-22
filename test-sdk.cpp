//
// Created by Giuseppe Coviello on 10/22/21.
//

#include "datax.h"

#include <iostream>

int main() {
  auto dx = datax::New();

  std::cout << "Configuration: " << dx->Configuration() << std::endl;
}