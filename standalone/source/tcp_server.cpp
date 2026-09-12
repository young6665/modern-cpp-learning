#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

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

   while (true) {
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
            closesocket(server_socket);
            WSACleanup();
            return 1;
        }

        if (bytes_received == 0) {
            std::cout << "Client disconnected.\n";
            break;
        }

        buffer[bytes_received] = '\0';

        std::string message(
            buffer,
            bytes_received
        );

        std::cout << "Message received: "
                << message
                << '\n';

        if (message == "/quit") {
            std::cout << "Client requested disconnection.\n";
            break;
        }

        int bytes_sent = send(
            client_socket,
            buffer,
            bytes_received,
            0
        );

        if (bytes_sent == SOCKET_ERROR) {
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
    }

    closesocket(client_socket);
    closesocket(server_socket);
    WSACleanup();

    return 0;
}