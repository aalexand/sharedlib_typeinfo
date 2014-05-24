#include "api.hpp"
#include <new>
#include <iostream>
#include <typeinfo>

class JobExecutorImpl : public JobExecutor {
 public:
  virtual void execute(const Job* job) {
    try {
      job->work();
    } catch (const SomeException& e) {
      std::cout << "Caught SomeException, &typeid(SomeException)="
          << &typeid(SomeException) << std::endl;
    } catch (...) {
      std::cout << "Caught unknown exception type via (...) fallback" << std::endl;
    }
  }
};

extern "C" JobExecutor* create_job_executor() {
  return new JobExecutorImpl();
}

