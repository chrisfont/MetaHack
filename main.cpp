#include "stdafx.h"

#include "App.h"
#include "ErrorHandler.h"
#include "New.h"

INITIALIZE_EASYLOGGINGPP

int main(int argc, char* argv[])
{
  START_EASYLOGGINGPP(argc, argv);

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
        throw std::exception("Shaders are not available on this platform");
      }

      // Create and open the main window.
      app_window.reset(NEW sf::RenderWindow(sf::VideoMode(1066, 600), "Magicule Saga"));

      // Create and run the app instance.
      app.reset(NEW App(*(app_window.get())));
      app->run();
    }
#ifdef NDEBUG
    catch (std::exception& e)
    {
      FATAL_ERROR("Caught top-level exception: %s", e.what());
    }
#endif
  }

  return EXIT_SUCCESS;
}