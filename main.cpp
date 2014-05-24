
#include "api.hpp"
#include <cassert>
#include <iostream>
#include <dlfcn.h>

typedef Job* (*JobCreateFn)();
typedef JobExecutor* (*JobExecutorCreateFn)();

int main(int argn, char* argv[]) {
  if (argn != 2 || (strcmp(argv[1], "local") && strcmp(argv[1], "global"))) {
    std::cerr << "usage: main <local | global>" << std::endl;
    return 1;
  }

  int flag = (strcmp(argv[1], "local") ? RTLD_GLOBAL : RTLD_LOCAL) | RTLD_NOW;

  void *h1 = dlopen("./libjob_impl.so", flag);
  assert(h1);
  JobCreateFn job_create_fn = (JobCreateFn)dlsym(h1, "create_job");
  assert(job_create_fn);
  Job* job = job_create_fn();
  assert(job);

  void *h2 = dlopen("./libjob_executor_impl.so", flag);
  assert(h2);
  JobExecutorCreateFn job_executor_create_fn = (JobExecutorCreateFn)dlsym(h2, "create_job_executor");
  assert(job_executor_create_fn);
  JobExecutor* job_executor = job_executor_create_fn();
  assert(job_executor);

  job_executor->execute(job);
}
