#include <datax.h>

int64_t currentTime() {
  return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count());
}

int main() {
  auto dx = datax::New();
  int64_t nextTime = 0;
  int64_t previousNextTime = 0;
  int64_t emitTime = 0;
  int64_t previousEmitTime = 0;
  int64_t latestReport = 0;
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
      printf("Latest report: %lld\n", latestReport);
      if (latestReport > 0) {
        auto elapsedTime = now - latestReport;

        auto nextTimeTaken = nextTime - previousNextTime;
        auto emitTimeTaken = emitTime - previousEmitTime;
        auto processingTimeTaken = elapsedTime - nextTimeTaken - emitTimeTaken;

        printf(
            "Time taken for 'next()': %lld ms (%0.2f%%), processing: %lld ms (%0.2f%%), 'emit()': %lld ms (%0.2f%%)\n",
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
      fflush(stdout);
    }
  }
}