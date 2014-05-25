GCC typeinfo and shared libraries
=================================

Catching a C++ exception thrown in another shared library or making a 
`dynamic_cast` for an object created in another shared library can be tricky in 
certain cases on Linux.  Here I give an illustration of such trickiness using 
exception handling as example.

The test code contains of an executable and two shared libraries:
* One shared library (`job_impl.cpp`) implements Job interface with 
  `JobImpl::work()` method that throws `SomeException` exception in this 
  particular implementation.
* Another shared library (`job_executor_impl.cpp`) implements JobExecutor 
  interface with `JobExecutorImpl::execute(const Job& job)` method that attempts 
  to catch `SomeException` exception and has also a `catch (...)` callback that 
  should never be reached in the test implementation since we only throw 
  `SomeException`.
* Executable binary (`main.cpp`) loads both shared libraries using `dlopen()`, 
  `dlsym()'s` into the libraries to create instances of the job and job executor 
  and then invokes the job executor's `JobExecutor::execute()` function passing 
  it the instance of the job object.

So, the code is quite straightforward, but interesting things happen when 
`dlopen` is used with `RTLD_LOCAL` flag as opposed to `RTLD_GLOBAL`.  The thrown 
exception doesn't get caught when the code is compiled with pre-4.5 GCC 
compiler!  It is caught as expected with GCC 4.5 and newer.  What happens here?

When `RTLD_LOCAL` flag is used to load a shared library, the dynamic linker does 
not expose dynamic symbols found in the shared library to global scope.  So, if 
two shared libraries export a dynamic symbol with the same name, each of them 
will get its own copy of the symbol rather than both names be resolved to the 
same symbol as with `RTLD_GLOBAL`.  This local resolution is often used when 
loading shared libraries in plugin infrastructures since it provides a basic 
isolation between the libraries.  There is one problem with `RTLD_LOCAL` though in 
terms of exception handling and dynamic_cast.  When compiler generates code for 
constructs like

```
  try {
    // Some code that may throw an std::exception-based exception.
  } catch (const Exception1& e) {
    // Handle Exception1 kind of errors.
  } catch (const Exception2& e) {
    // Handle Exception2 kind of errors.
  } catch (...) {
    // Handle other kinds of errors.
  }
```

it needs to know the dynamic type of the exception being handled in order to 
know which catch block it should be handled in.  You can think of it as series 
of conditional `dynamic_cast` statements - this is why the problem with `RTLD_LOCAL` 
has consequences for both cross-shared-library exceptions and dynamic_cast's.  
The check of dynamic type is done by comparing `typeid()` of the object with the 
`typeid()` of the candidate dynamic type.  So, the code above can conceptually be 
translated to code below (assuming for simplicity that there is no virtual 
inheritance in our exception hierarchy and simple `static_cast` works fine to 
downcast the types):

```
  try {
    // Some code that may throw an std::exception-based exception.
  } catch (const std::exception& thrown) {
    if (typeid(thrown) == typeid(Exception1)) {
      const Exception1& e = static_cast<const Exception1&>(thrown);
      // Handle Exception1 kind of errors.
    } else if (typeid(thrown) == typeid(Exception2)) {
      const Exception2& e = static_cast<const Exception2&>(thrown);
      // Handle Exception2 kind of errors.
    } else {
      // Handle other kinds of errors.
    }
  }
```

It is important here how `std::type_info` objects (returned by `typeid()`) are 
checked for equality.  The default behavior on systems with weak linker symbols 
enabled and pre-4.5 version of GCC is to check `type_info` objects for equality by comparing 
their pointers.  This assumes that the dynamic linker is going to bind all 
references to a given `std::type_info` object to the same instance.  This is a 
reasonable expectation in case of automatic loading of shared libraries or with 
explicit loading using `dlopen()` with `RTLD_GLOBAL` flag.  But not with `RTLD_LOCAL` 
since, as we saw, this flag will make each shared library loaded in such way 
have its own copy of the `std::type_info` object for each type.  This is the 
reason why in the test example the exception is not caught - it simply fall 
through since the pointers are different.

This behavior existed for long time, but with GCC 4.5 and newer the default has 
changed to check `std::type_info` objects for equality using their names rather 
than pointers.  Actually, this mode is available in the older GCC versions as 
well, but to enable it you need to recompile GCC with 
`__GXX_MERGED_TYPEINFO_NAMES` define set to 1.  Also, some Linux distributions 
seem to have backported the GCC 4.5. change in the default behavior to the 
earlier versions of GCC that they actually use.  For example, RHEL6 that uses 
GCC 4.4.6 has the change and uses the name-based equality check for 
`std::type_info`.

To conclude, catching a C++ exception thrown in another shared library or making 
a `dynamic_cast` for an object created in another shared library can be tricky in 
case of using `dlopen()` with the default `RTLD_LOCAL` flag.  To overcome the 
issue with multiple instances of `std::type_info` for the same type you can do one 
of the following:
* Use a version of GCC compiler of 4.5 or newer.  You might also be able to get 
  by with a 4.4 version on a distro that backported the change in `std::type_info` 
  equality check from 4.5.
* Recompile the older version of GCC that you use with 
  `__GXX_MERGED_TYPEINFO_NAMES` define set to 1.  This option is generally a 
  hassle.
* Use `RTLD_GLOBAL` for all `dlopen()` calls in your code.  This may reduce 
  symbol name isolation between your plugins but may be OK in many cases so this 
  option should be on the table.
* Avoid the need to throw exceptions across shared library boundaries and having 
  to do `dynamic_cast` for objects created in other shared libraries.  This option 
  is my favourite one since it makes things working everywhere.

vim: set tw=80 ai fo+=a2qwt :
