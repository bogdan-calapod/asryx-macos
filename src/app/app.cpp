#include "app/app.hpp"

#include "install/install.hpp"
#include "model/model.hpp"
#include "runtime/runtime.hpp"

#include <iostream>
#include <stdexcept>

namespace app {

namespace {

void print_usage()
{
  std::cerr
      << "asryx - Voice-to-Text Toggle\n\n"
      << "Usage:\n"
      << "  asryx                           Toggle recording/transcription\n"
      << "  asryx status                    Print current status (idle, recording, transcribing)\n"
      << "  asryx uninstall                 Uninstall all asryx components\n"
      << "  asryx --model list              List all available model sizes\n"
      << "  asryx --model install <name>    Install a model\n"
      << "  asryx --model use <name>        Select active model\n"
      << "  asryx --model uninstall <name>  Uninstall a model\n";
}

} // namespace

int run(const std::vector<std::string>& args)
{
  try {
    if (args.empty()) {
      runtime::toggle();
      return 0;
    }

    if (args.size() == 1) {
      if (args[0] == "status") {
        std::cout << runtime::get_status() << "\n";
        return 0;
      }
      if (args[0] == "uninstall") {
        install::uninstall_app();
        return 0;
      }
    }

    if (args.size() == 2 && args[0] == "--model") {
      if (args[1] == "list") {
        model::list_models();
        return 0;
      }
    }

    if (args.size() == 3 && args[0] == "--model") {
      if (args[1] == "install") {
        model::install_model(args[2]);
        return 0;
      }
      if (args[1] == "use") {
        model::use_model(args[2]);
        return 0;
      }
      if (args[1] == "uninstall") {
        model::uninstall_model(args[2]);
        return 0;
      }
    }

    std::cerr << "error: invalid arguments\n\n";
    print_usage();
    return 1;
  }
  catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
}

} // namespace app
