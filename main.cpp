#include "stdafx.h"

#include "App.h"
#include "ErrorHandler.h"

INITIALIZE_EASYLOGGINGPP

int main(int argc, char* argv[])
{
  START_EASYLOGGINGPP(argc, argv);

  el::Loggers::reconfigureAllLoggers(
    el::ConfigurationType::Format,
    "[%logger] %loc %level: %msg");

  LOG(INFO) << "Entered main()";

  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

  // Scoping everything here so it is properly destroyed before checking for memory leaks.
  {
    std::unique_ptr<sf::RenderWindow> app_window;
    std::unique_ptr<App> app;

#ifdef NDEBUG
    try
#endif
    {
      // Check to make sure shaders are available.
      if (!sf::Shader::isAvailable())
      {
        LOG(FATAL) << "Shaders are not available on this platform";
      }

      // Create and open the main window.
      app_window.reset(NEW sf::RenderWindow(sf::VideoMode(1066, 600), "Magicule Saga"));

      auto settings = app_window->getSettings();
      LOG(INFO) << "depthBits = " << settings.depthBits;
      LOG(INFO) << "stencilBits = " << settings.stencilBits;
      LOG(INFO) << "antialiasingLevel = " << settings.antialiasingLevel;
      LOG(INFO) << "version = " << settings.majorVersion << "." << settings.minorVersion;
      LOG(INFO) << "attributeFlags = 0x" << hexify<uint32_t>(settings.attributeFlags);

      // Create and run the app instance.
      app.reset(NEW App(*app_window));
      app->run();
    }
#ifdef NDEBUG
    catch (std::exception& e)
    {
      LOG(FATAL) << "Caught top-level exception: " << e.what();
    }
#endif
  }

  return EXIT_SUCCESS;
}