#include <minichat/command.h>

namespace minichat {

Command parse_command(const std::string& content) {
    if (content == "/quit") {
        return Command::quit;
    }

    if (content == "/list") {
        return Command::list;
    }

    if(content == "/count") {
        return Command::count;
    }

    if(content == "/clear") {
        return Command::clear;
    }

    if(content == "/help") {
        return Command::help;
    }

    return Command::none;
}

}