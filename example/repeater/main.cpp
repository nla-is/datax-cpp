#include <datax.h>

uint64_t currentTime() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

int main() {
  auto dx = datax::New();
  uint64_t nextTime = 0;
  uint64_t previousNextTime = 0;
  uint64_t emitTime = 0;
  uint64_t previousEmitTime = 0;
  uint64_t latestReport = 0;
  while (true) {
    auto start = currentTime();
    auto msg = dx->Next();
    nextTime += currentTime() - start;
    msg.Data["repeated"] = true;
    start = currentTime();
    dx->Emit(msg.Data, msg.Reference);
    auto now = currentTime();
    emitTime += now - start;
    if (now - latestReport > 10000) {
      if (latestReport > 0) {
        auto elapsedTime = now - latestReport;

        auto nextTimeTaken = nextTime - previousNextTime;
        auto emitTimeTaken = emitTime - previousEmitTime;
        auto processingTimeTaken = elapsedTime - nextTimeTaken - emitTimeTaken;

        printf(
            "Time taken for 'next()': %llu ms (%0.2f%%), processing: %llu ms (%0.2f%%), 'emit()': %llu ms (%0.2f%%)\n",
            nextTimeTaken,
            (((double) nextTimeTaken) / ((double) elapsedTime)) * 100,
            processingTimeTaken,
            (((double) processingTimeTaken) / ((double) elapsedTime)) * 100,
            emitTimeTaken,
            (((double) emitTimeTaken) / ((double) elapsedTime)) * 100);
      }

      previousNextTime = nextTime;
      previousEmitTime = emitTime;

      latestReport = now;
    }
  }
}