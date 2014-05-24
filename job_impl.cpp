
#include "api.hpp"
#include <new>
#include <typeinfo>
#include <iostream>

class JobImpl : public Job {
  virtual void work() const {
    std::cout << "Throw: &typeid(SomeException)=" << &typeid(SomeException) << std::endl;
    throw SomeException();
  }
};

extern "C" Job* create_job() {
  return new JobImpl();
}

