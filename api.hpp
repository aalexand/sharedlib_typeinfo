#pragma once

#include <exception>

class Job {
 public:
  virtual void work() const = 0;
  virtual ~Job() {}
};

class JobExecutor {
 public:
  virtual void execute(const Job* job) = 0;
  virtual ~JobExecutor() {}
};

class SomeException : public std::exception {
};
