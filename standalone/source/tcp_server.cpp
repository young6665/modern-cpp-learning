#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
#include <atomic>

std::mutex output_mutex;

std::vector<SOCKET> connected_clients;
std::mutex clients_mutex;

std::atomic<bool> server_running{true};

void log_message(const std::string& text) {
    std::lock_guard<std::mutex> lock(output_mutex);

    std::cout << text << '\n';
}

void log_error(const std::string& text) {
    std::lock_guard<std::mutex> lock(output_mutex);

    std::cerr << text << '\n';
}

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

void add_client(SOCKET client_socket){
    std::lock_guard<std::mutex> lock(clients_mutex);
    connected_clients.push_back(client_socket);
}

void remove_client(SOCKET client_socket) {
    std::lock_guard<std::mutex> lock(clients_mutex);

    auto client_position = std::find(
        connected_clients.begin(),
        connected_clients.end(),
        client_socket
    );

    if (client_position != connected_clients.end()) {
        connected_clients.erase(client_position);
    }
}

std::size_t get_client_count() {
    std::lock_guard<std::mutex> lock(clients_mutex);

    return connected_clients.size();
}

void broadcast_message(const std::string& message){
    std::string message_packet = message + '\n';

    std::lock_guard<std::mutex> lock(clients_mutex);
    for(SOCKET target_socket : connected_clients){
        bool send_success = send_all(
            target_socket,
            message_packet.c_str(),
            static_cast<int>(message_packet.size())
        );
        if (!send_success) {
            log_error(
                "Broadcast send failed: " +
                std::to_string(WSAGetLastError())
            );

        }
    }   

}

void shutdown_all_clients(){
    std::lock_guard<std::mutex> lock(clients_mutex);

    for(SOCKET client_socket : connected_clients){
        shutdown(client_socket, SD_BOTH);
    }
}

void handle_client(
    SOCKET client_socket,
    int client_id
) {
    log_message(
        "[Client " +
        std::to_string(client_id) +
        "] handler started."
    );

    std::string pending_data;

    while (true) {
        std::string message;

        ReceiveStatus receive_status = receive_line(
            client_socket,
            pending_data,
            message
        );

        if (receive_status == ReceiveStatus::error) {
            log_error(
                "[Client " +
                std::to_string(client_id) +
                "] receive failed: " +
                std::to_string(WSAGetLastError())
            );
            break;
        }

        if (receive_status ==
            ReceiveStatus::disconnected) {
            log_message(
                "[Client " +
                std::to_string(client_id) +
                "] disconnected."
            );
            break;
        }

        log_message(
            "[Client " +
            std::to_string(client_id) +
            "] message: " +
            message
        );

        if (message == "/quit") {
            log_message(
                "[Client " +
                std::to_string(client_id) +
                "] requested disconnection."
            );
            break;
        }

        if (message == "/shutdown") {
            log_message(
                "[Client " +
                std::to_string(client_id) +
                "] requested server shutdown."
            );
            broadcast_message("[Server] shutting down.");

            server_running = false;
            break;
        }


        std::string echo_packet = message + '\n';

        std::string chat_message =
        "[Client " +
        std::to_string(client_id) +
        "] " +
        message;

        broadcast_message(chat_message);

        log_message(
            "[Client " +
            std::to_string(client_id) +
            "] message broadcasted."
        );  
    }
        remove_client(client_socket);
        closesocket(client_socket);

        log_message(
            "[Client " +
            std::to_string(client_id) +
            "] connection closed. Online clients: " +
            std::to_string(get_client_count())
        );
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
    
    std::vector<std::thread> client_threads;
    int next_client_id = 1;
    log_message("Waiting for clients...");

    while (server_running) {
        fd_set read_sockets;
        FD_ZERO(&read_sockets);
        FD_SET(server_socket, &read_sockets);

        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = 200000; // 200 毫秒

        int ready_count = select(
            0,
            &read_sockets,
            nullptr,
            nullptr,
            &timeout
        );

        if (ready_count == SOCKET_ERROR) {
            log_error(
                "select failed: " +
                std::to_string(WSAGetLastError())
            );

            server_running = false;
            break;
        }

        // 200 毫秒内没有新连接，回到 while 顶部检查 server_running
        if (ready_count == 0) {
            continue;
        }

        if (!server_running) {
            break;
        }

        if (!FD_ISSET(server_socket, &read_sockets)) {
            continue;
        }

        sockaddr_in client_address{};
        int client_address_length = sizeof(client_address);

        SOCKET client_socket = accept(
            server_socket,
            reinterpret_cast<sockaddr*>(&client_address),
            &client_address_length
        );

        if (client_socket == INVALID_SOCKET) {
            log_error(
                "accept failed: " +
                std::to_string(WSAGetLastError())
            );

            continue;
        }

        // 防止 /shutdown 与 accept 恰好同时发生
        if (!server_running) {
            closesocket(client_socket);
            break;
        }

        int client_id = next_client_id++;

        add_client(client_socket);

        log_message(
            "Client " +
            std::to_string(client_id) +
            " connected. Online clients: " +
            std::to_string(get_client_count())
        );

        client_threads.emplace_back(
            handle_client,
            client_socket,
            client_id
        );

        log_message(
        "Stored thread objects: " +
        std::to_string(client_threads.size())
        );
    }
    // main 线程不再使用监听套接字，可以安全关闭
    closesocket(server_socket);

    // 让所有阻塞在 recv() 的客户端线程退出
    shutdown_all_clients();

    for (std::thread& client_thread : client_threads) {
        if (client_thread.joinable()) {
            client_thread.join();
        }
    }

    WSACleanup();

    log_message("Server stopped.");
    return 0;
}