#include <datax.h>

int main() {
  auto dx = datax::New();
  while (true) {
    auto msg = dx->Next();
    msg.Data["repeated"] = true;
    dx->Emit(msg.Data, msg.Reference);
  }
}