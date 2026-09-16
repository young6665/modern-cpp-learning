#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include <fstream>
#include <exception>
#include <stdexcept>

struct ClientConfig {
    std::string server_ip_address =
        "127.0.0.1";

    unsigned short server_port = 8080;
};
std::atomic<bool> client_running{true};
std::mutex client_output_mutex;

bool send_all(
    SOCKET socket_handle,
    const char* data,
    int data_length
) {
    int total_sent = 0;

    while (total_sent < data_length) {
        int bytes_sent = send(
            socket_handle,
            data + total_sent,
            data_length - total_sent,
            0
        );

        if (bytes_sent == SOCKET_ERROR ||
            bytes_sent == 0) {
            return false;
        }

        total_sent += bytes_sent;
    }

    return true;
}

enum class ReceiveStatus {
    success,
    disconnected,
    error
};

ReceiveStatus receive_line(
    SOCKET socket_handle,
    std::string& pending_data,
    std::string& message
) {
    while (true) {
        std::size_t newline_position =
            pending_data.find('\n');

        if (newline_position != std::string::npos) {
            message = pending_data.substr(
                0,
                newline_position
            );

            pending_data.erase(
                0,
                newline_position + 1
            );

            return ReceiveStatus::success;
        }

        char buffer[1024]{};

        int bytes_received = recv(
            socket_handle,
            buffer,
            sizeof(buffer),
            0
        );

        if (bytes_received == 0) {
            return ReceiveStatus::disconnected;
        }

        if (bytes_received == SOCKET_ERROR) {
            return ReceiveStatus::error;
        }

        pending_data.append(
            buffer,
            bytes_received
        );
    }
}

void print_client_message(const std::string& text) {
    std::lock_guard<std::mutex> lock(
        client_output_mutex
    );

    std::cout << text << '\n';
}

void print_client_error(const std::string& text) {
    std::lock_guard<std::mutex> lock(
        client_output_mutex
    );

    std::cerr << text << '\n';
}

std::string trim(const std::string& text) {
    std::size_t first =
        text.find_first_not_of(
            " \t\r\n"
        );

    if (first == std::string::npos) {
        return "";
    }

    std::size_t last =
        text.find_last_not_of(
            " \t\r\n"
        );

    return text.substr(
        first,
        last - first + 1
    );
}

bool load_client_config(
    const std::string& file_path,
    ClientConfig& config
) {
    std::ifstream config_file(
        file_path
    );

    if (!config_file.is_open()) {
        print_client_error(
            "Could not open client configuration file: " +
            file_path
        );

        return false;
    }

    ClientConfig loaded_config = config;

    std::string line;
    int line_number = 0;

    while (std::getline(config_file, line)) {
        ++line_number;

        line = trim(line);

        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::size_t equal_position =
            line.find('=');

        if (equal_position ==
            std::string::npos) {
            print_client_error(
                "Invalid client configuration line " +
                std::to_string(line_number) +
                ": missing ="
            );

            return false;
        }

        std::string key = trim(
            line.substr(
                0,
                equal_position
            )
        );

        std::string value = trim(
            line.substr(
                equal_position + 1
            )
        );

        try {
            if (key == "server_ip_address") {
                if (value.empty()) {
                    throw std::invalid_argument(
                        "empty IP address"
                    );
                }

                loaded_config.server_ip_address =
                    value;
            } else if (key == "server_port") {
                int port = std::stoi(value);

                if (port < 1 || port > 65535) {
                    throw std::out_of_range(
                        "invalid port"
                    );
                }

                loaded_config.server_port =
                    static_cast<unsigned short>(
                        port
                    );
            } else {
                print_client_error(
                    "Unknown client configuration key: " +
                    key
                );

                return false;
            }
        } catch (const std::exception&) {
            print_client_error(
                "Invalid client configuration value on line " +
                std::to_string(line_number)
            );

            return false;
        }
    }

    config = loaded_config;
    return true;
}


void receive_messages(SOCKET client_socket) {
    std::string pending_data;

    while (client_running) {
        std::string message;

        ReceiveStatus receive_status = receive_line(
            client_socket,
            pending_data,
            message
        );

        if (receive_status == ReceiveStatus::error) {
            if (client_running) {
                print_client_error(
                    "Receive failed: " +
                    std::to_string(WSAGetLastError())
                );
            }

            break;
        }

        if (receive_status ==
            ReceiveStatus::disconnected) {
            if (client_running) {
                print_client_message(
                    "Server disconnected."
                );
            }

            break;
        }

        {
            std::lock_guard<std::mutex> lock(
                client_output_mutex
            );

            std::cout
                    << "Message received: "
                    << message
                    << '\n'
                    << '\n'
                    << "Enter message: "
                    << std::flush;
        }
    }

    client_running = false;
}



int main() {
    ClientConfig config;
    bool config_loaded =
        load_client_config(
            "standalone/client.conf",
            config
        );

    if (config_loaded) {
        print_client_message(
            "Client configuration loaded successfully."
        );
    } else {
        print_client_error(
            "Using default client configuration."
        );
    }

    print_client_message(
        "Server address: " +
        config.server_ip_address +
        ":" +
        std::to_string(
            config.server_port
        )
    );


    WSADATA wsa_data{};

    int result = WSAStartup(
        MAKEWORD(2, 2),
        &wsa_data
    );

    if (result != 0) {
        std::cerr << "WSAStartup failed: "
                  << result
                  << '\n';
        return 1;
    }

    std::cout << "Winsock initialized successfully.\n";

    SOCKET client_socket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: "
                  << WSAGetLastError()
                  << '\n';

        WSACleanup();
        return 1;
    }

    std::cout << "Client socket created successfully.\n";

    sockaddr_in server_address{};

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(config.server_port);

    int address_result = inet_pton(
        AF_INET,
        config.server_ip_address.c_str(),
        &server_address.sin_addr
    );

    if (address_result != 1) {
        std::cerr << "Invalid server IP address.\n";

        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connecting to server...\n";

    int connect_result = connect(
        client_socket,
        reinterpret_cast<sockaddr*>(&server_address),
        sizeof(server_address)
    );

    if (connect_result == SOCKET_ERROR) {
        std::cerr << "Connect failed: "
                  << WSAGetLastError()
                  << '\n';

        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server successfully.\n";

    client_running = true;

    std::thread receiver_thread(
        receive_messages,
        client_socket
    );
          
    {
        std::lock_guard<std::mutex> lock(
                client_output_mutex
            );
        std::cout << "Enter message: "
                    << std::flush;
        
    }
    while (client_running) {
 

        std::string message;

        if (!std::getline(std::cin, message)) {
            break;
        }

        if (!client_running) {
            break;
        }

        if (message.empty()) {
            print_client_message(
                "Message cannot be empty."
            );
            continue;
        }

        std::string message_packet = message + '\n';

        bool send_success = send_all(
            client_socket,
            message_packet.c_str(),
            static_cast<int>(message_packet.size())
        );

        if (!send_success) {
            print_client_error(
                "Send failed: " +
                std::to_string(WSAGetLastError())
            );

            client_running = false;
            break;
        }

        print_client_message(
            "Message sent successfully: " + message
        );

        if (message == "/quit") {
            print_client_message(
                "Disconnecting from server."
            );

            client_running = false;
            break;
        }

    }

    shutdown(
    client_socket,
    SD_SEND
    );

    if (receiver_thread.joinable()) {
        receiver_thread.join();
    }

    closesocket(client_socket);
    WSACleanup();

    return 0;
}