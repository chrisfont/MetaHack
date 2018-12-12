#include "stdafx.h"

#include "services/DefaultPaths.h"

#include <boost/filesystem.hpp>

#ifdef __APPLE__
#include "CoreFoundation/CoreFoundation.h"
#endif

DefaultPaths::DefaultPaths()
{
  std::string workingDirectory { "." /* boost::filesystem::current_path().string() */ };
  std::string logDirectory { workingDirectory + "/log" };
  std::string resourcesDirectory { workingDirectory + "/resources" };

  // If in MacOS, we get these in a somewhat different manner, because Apple has to be different.
  // (See https://stackoverflow.com/questions/516200/relative-paths-not-working-in-xcode-c?rq=1 for details)
#ifdef __APPLE__
  CFBundleRef mainBundle = CFBundleGetMainBundle();
  char path[PATH_MAX];

  CFURLRef executableURL = CFBundleCopyExecutableURL(mainBundle);
  if (!CFURLGetFileSystemRepresentation(executableURL, TRUE, (UInt8 *)path, PATH_MAX))
  {
    LOG(FATAL) << "Could not obtain resources directory name from CoreFoundation!";
  }
  logDirectory = std::string(path) + "/log";
  CFRelease(executableURL);

  CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
  if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
  {
    LOG(FATAL) << "Could not obtain resources directory name from CoreFoundation!";
  }
  resourcesDirectory = std::string(path);
  CFRelease(resourcesURL);
#endif

  std::cout << "Log directory is " << logDirectory << std::endl;
  std::cout << "Resources directory is " << resourcesDirectory << std::endl;

  m_logsPath = logDirectory;
  m_resourcesPath = resourcesDirectory;
}

DefaultPaths::~DefaultPaths()
{
  //dtor
}

std::string const& DefaultPaths::resources()
{
  return m_resourcesPath;
}

std::string const& DefaultPaths::logs()
{
  return m_logsPath;
}
