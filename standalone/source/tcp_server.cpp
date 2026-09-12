#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

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

enum class ReceiveStatus{
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

    SOCKET server_socket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: "
                  << WSAGetLastError()
                  << '\n';

        WSACleanup();
        return 1;
    }

    std::cout << "Socket created successfully.\n";

    sockaddr_in server_address{};

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);

    int address_result = inet_pton(
        AF_INET,
        "127.0.0.1",
        &server_address.sin_addr
    );

    if (address_result != 1) {
        std::cerr << "Invalid IP address.\n";

        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    int bind_result = bind(
        server_socket,
        reinterpret_cast<sockaddr*>(&server_address),
        sizeof(server_address)
    );

    if (bind_result == SOCKET_ERROR) {
        std::cerr << "Bind failed: "
                  << WSAGetLastError()
                  << '\n';

        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Socket bound to 127.0.0.1:8080 successfully.\n";

    int listen_result = listen(server_socket, 1);

    if (listen_result == SOCKET_ERROR) {
        std::cerr << "Listen failed: "
                << WSAGetLastError()
                << '\n';

        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is listening on 127.0.0.1:8080...\n";
    std::cout << "Waiting for a client...\n";

    SOCKET client_socket = accept(
        server_socket,
        nullptr,
        nullptr
    );

    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Accept failed: "
                << WSAGetLastError()
                << '\n';

        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Client connected successfully.\n";

std::string pending_data;

    while (true) {
        std::string message;

        ReceiveStatus receive_status = receive_line(
            client_socket,
            pending_data,
            message
        );

        if (receive_status == ReceiveStatus::error) {
            std::cerr << "Receive failed: "
                    << WSAGetLastError()
                    << '\n';

            closesocket(client_socket);
            closesocket(server_socket);
            WSACleanup();
            return 1;
        }

        if (receive_status == ReceiveStatus::disconnected) {
            std::cout << "Client disconnected.\n";
            break;
        }

        std::cout << "Message received: "
                << message
                << '\n';

        if (message == "/quit") {
            std::cout << "Client requested disconnection.\n";
            break;
        }

        std::string echo_packet = message + '\n';

        bool echo_success = send_all(
            client_socket,
            echo_packet.c_str(),
            static_cast<int>(echo_packet.size())
        );

        if (!echo_success) {
            std::cerr << "Echo send failed: "
                    << WSAGetLastError()
                    << '\n';

            closesocket(client_socket);
            closesocket(server_socket);
            WSACleanup();
            return 1;
        }

        std::cout << "Echo sent successfully: "
                << message
                << '\n';

        std::cout << "Echo bytes sent: "
                << echo_packet.size()
                << '\n';
    }

    closesocket(client_socket);
    closesocket(server_socket);
    WSACleanup();

    return 0;

    closesocket(client_socket);
    closesocket(server_socket);
    WSACleanup();

    return 0;
}