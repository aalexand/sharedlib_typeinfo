CXX=/opt/gcc-4.1.2/bin/g++
CXXLIBDIR=$(shell dirname `$(CXX) -print-file-name=libstdc++.so`)
LDFLAGS=-Wl,-rpath=$(CXXLIBDIR)

all : main libjob_impl.so libjob_executor_impl.so

main : main.cpp
	$(CXX) -g -o $@ $< -ldl $(LDFLAGS)

libjob_impl.so : job_impl.cpp
	$(CXX) -g -fPIC -shared -o $@ $< $(LDFLAGS)

libjob_executor_impl.so : job_executor_impl.cpp
	$(CXX) -g -fPIC -shared -o $@ $< $(LDFLAGS)

clean:
	rm -f main *.so *.o
