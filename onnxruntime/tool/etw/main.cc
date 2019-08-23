// Turns the DEFINE_GUID for EventTraceGuid into a const.
#include "eparser.h"
#include "TraceSession.h"

#define INITGUID

static const GUID OrtProviderGuid = {0x54d81939, 0x62a0, 0x4dc0, {0xbf, 0x32, 0x3, 0x5e, 0xbd, 0xc7, 0xbc, 0xe9}};

int real_main(int argc, TCHAR* argv[]) {
  ProfilingInfo context;
  TraceSession session;
  session.AddHandler(OrtProviderGuid, OrtEventHandler, &context);
  session.InitializeEtlFile(L"C:\\src\\onnxruntime\\1.etl", nullptr);
  ULONG status = ProcessTrace(&session.traceHandle_, 1, 0, 0);
  if (status != ERROR_SUCCESS && status != ERROR_CANCELLED) {    
    std::cout << "OpenTrace failed with " << status << std::endl;
	session.Finalize();
    return -1;
  }
  session.Finalize();

  assert(context.ortrun_count == context.ortrun_end_count);
  std::vector<OpStat*> stat_array(context.op_stat.size());
  size_t i = 0;
  for (auto& p : context.op_stat) {
    stat_array[i++] = &p.second;
  }
  std::sort(stat_array.begin(), stat_array.end(),
            [](const OpStat* left, const OpStat* right) { return left->total_time > right->total_time; });
  size_t iterations = context.time_per_run.size();
  ULONG64 total_time = std::accumulate(context.time_per_run.begin() + 1, context.time_per_run.end(), (ULONG64)0);
  // in microseconds
  ULONG64 avg_time = total_time / (context.time_per_run.size() - 1) / 10;
  double sum = 0;
  for (OpStat* p : stat_array) {
    if (p->name == L"Scan") {
      continue;
    }
    uint64_t avg_time_per_op = p->total_time / iterations;
    if (avg_time_per_op >= 0) {
      double t = avg_time_per_op * 100.0 / avg_time;
      std::wcout << p->name << L" " << p->total_time / p->count << L" " << std::fixed << std::setprecision(1) << t
                 << L"%\n";
    }
    sum += p->total_time / (double)iterations;
  }
  std::wcout << L"total  " << std::fixed << std::setprecision(1) << (sum * 100.0) / avg_time << L"%\n";
  return 0;
}

int _tmain(int argc, TCHAR* argv[]) {
  int retval = -1;
  try {
    retval = real_main(argc, argv);
  } catch (std::exception& ex) {
    fprintf(stderr, "%s\n", ex.what());
    retval = -1;
  }
  return retval;
}