#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

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

int main() {
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
    server_address.sin_port = htons(8080);

    int address_result = inet_pton(
        AF_INET,
        "127.0.0.1",
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

   std::string pending_data;

    while (true) {
        std::cout << "Enter message: ";

        std::string message;
        std::getline(std::cin, message);

        if (message.empty()) {
            std::cout << "Message cannot be empty.\n";
            continue;
        }

        std::string message_packet = message + '\n';

        bool send_success = send_all(
            client_socket,
            message_packet.c_str(),
            static_cast<int>(message_packet.size())
        );

        if (!send_success) {
            std::cerr << "Send failed: "
                    << WSAGetLastError()
                    << '\n';

            closesocket(client_socket);
            WSACleanup();
            return 1;
        }

        std::cout << "Message sent successfully: "
                << message
                << '\n';

        std::cout << "Packet bytes sent: "
                << message_packet.size()
                << '\n';

        if (message == "/quit") {
            std::cout << "Disconnecting from server.\n";
            break;
        }

        std::string echo_message;

        ReceiveStatus receive_status = receive_line(
            client_socket,
            pending_data,
            echo_message
        );

        if (receive_status == ReceiveStatus::error) {
            std::cerr << "Receive failed: "
                    << WSAGetLastError()
                    << '\n';

            closesocket(client_socket);
            WSACleanup();
            return 1;
        }

        if (receive_status == ReceiveStatus::disconnected) {
            std::cout << "Server disconnected.\n";
            break;
        }

        std::cout << "Echo received: "
                << echo_message
                << '\n';
    }

    closesocket(client_socket);
    WSACleanup();

    return 0;
}