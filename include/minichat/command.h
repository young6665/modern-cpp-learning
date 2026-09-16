#pragma once

#include <string>

namespace minichat {

enum class Command {
    none,
    quit,
    list,
    count,
    clear,
    help
};

Command parse_command(const std::string& content);

}