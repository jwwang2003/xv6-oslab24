struct sysinfo {
  uint64 freemem;   // amount of free memory (bytes)
  uint64 nproc;     // number of process
};

struct sched_info{
    int wait_time; // 进程等待时间
    int cpu_time;  // 进程使用的CPU时间
    int sleep_time;
};