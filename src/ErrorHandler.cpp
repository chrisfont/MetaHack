#include "ErrorHandler.h"

#include <cstdlib>

struct ErrorHandler::Impl
{
    static std::unique_ptr<ErrorHandler> handler_instance;
    bool die_on_minor_error = false;
    bool die_on_major_error = false;
};

std::unique_ptr<ErrorHandler> ErrorHandler::Impl::handler_instance;

ErrorHandler::ErrorHandler()
{
  //ctor
}

ErrorHandler::~ErrorHandler()
{
  //dtor
}

ErrorHandler& ErrorHandler::instance()
{
  if (Impl::handler_instance.get() == nullptr)
  {
    Impl::handler_instance.reset(new ErrorHandler());
  }

  return *(Impl::handler_instance.get());
}

void ErrorHandler::printTrace(char* buf,
                              char const* file,
                              int line,
                              char const* func)
{
  printf("%s:%d (%s): %s\n", file, line, func, buf);
}


void ErrorHandler::handleMinorError(char* buf,
                                    char const* file,
                                    int line,
                                    char const* func)
{
  printf("%s:%d (%s): MINOR error: %s\n", file, line, func, buf);

  if (impl->die_on_minor_error)
  {
    printf("Dying because die_on_minor_error is true.\n");
    printf("Press Enter to exit.\n");
    getchar();
    exit(-3);
  }
}

void ErrorHandler::handleMajorError(char* buf,
                                    char const* file,
                                    int line,
                                    char const* func)
{
  printf("%s:%d (%s): MAJOR error: %s\n", file, line, func, buf);

  if (impl->die_on_major_error)
  {
    printf("Dying because die_on_major_error is true.\n");
    printf("Press Enter to exit.\n");
    getchar();
    exit(-2);
  }
}

void ErrorHandler::handleFatalError(char* buf,
                                    char const* file,
                                    int line,
                                    char const* func)
{
  printf("%s:%d (%s): FATAL error: %s\n", file, line, func, buf);

  printf("Dying because this is an unrecoverable error.\n");
  printf("Press Enter to exit.\n");
  getchar();
  exit(-1);
}

void ErrorHandler::setErrorHandlingFlags(bool minor, bool major)
{
  impl->die_on_minor_error = minor;
  impl->die_on_major_error = major;
}

