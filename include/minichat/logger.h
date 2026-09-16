#pragma once

#include <string>

void set_log_file_path(
    const std::string& file_path
);


void log_message(const std::string& text);

void log_error(const std::string& text);