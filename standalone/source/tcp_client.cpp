#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

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

    while (true) {
        std::cout << "Enter message: ";

        std::string message;
        std::getline(std::cin, message);

        if (message.empty()) {
            std::cout << "Message cannot be empty.\n";
            continue;
        }

        int bytes_sent = send(
            client_socket,
            message.c_str(),
            static_cast<int>(message.size()),
            0
        );

        if (bytes_sent == SOCKET_ERROR) {
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

        if (message == "/quit") {
            std::cout << "Disconnecting from server.\n";
            break;
        }

        char buffer[1024]{};

        int bytes_received = recv(
            client_socket,
            buffer,
            sizeof(buffer) - 1,
            0
        );

        if (bytes_received == SOCKET_ERROR) {
            std::cerr << "Receive failed: "
                    << WSAGetLastError()
                    << '\n';

            closesocket(client_socket);
            WSACleanup();
            return 1;
        }

        if (bytes_received == 0) {
            std::cout << "Server disconnected.\n";
            break;
        }

            buffer[bytes_received] = '\0';

            std::cout << "Echo received: "
                    << buffer
                    << '\n';
    }

    closesocket(client_socket);
    WSACleanup();

    return 0;
}