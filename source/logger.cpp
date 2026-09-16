#include <minichat/logger.h>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

// output_mutex、get_current_time()
// write_log()、log_message()、log_error()

std::mutex output_mutex;

std::string server_log_path =
    "standalone/server.log";

std::string get_current_time() {
    auto now =
        std::chrono::system_clock::now();

    std::time_t current_time =
        std::chrono::system_clock::to_time_t(
            now
        );

    std::tm local_time{};

    localtime_s(
        &local_time,
        &current_time
    );

    std::ostringstream time_stream;

    time_stream << std::put_time(
        &local_time,
        "%Y-%m-%d %H:%M:%S"
    );

    return time_stream.str();
}

void write_log(
    const std::string& level,
    const std::string& text,
    bool is_error
) {
    std::lock_guard<std::mutex> lock(
        output_mutex
    );

    std::string log_line =
        "[" +
        get_current_time() +
        "] [" +
        level +
        "] " +
        text;

    if (is_error) {
        std::cerr << log_line << '\n';
    } else {
        std::cout << log_line << '\n';
    }

    std::ofstream log_file(
        server_log_path,
        std::ios::app
    );

    if (!log_file.is_open()) {
        std::cerr
            << "Could not open server log file.\n";

        return;
    }

    log_file << log_line << '\n';
}

void set_log_file_path(
    const std::string& file_path
) {
    std::lock_guard<std::mutex> lock(
        output_mutex
    );

    server_log_path = file_path;
}


void log_message(const std::string& text) {
    write_log(
        "INFO",
        text,
        false
    );
}

void log_error(const std::string& text) {
    write_log(
        "ERROR",
        text,
        true
    );
}

