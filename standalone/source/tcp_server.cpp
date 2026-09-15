#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <unordered_map>

std::mutex output_mutex;
std::vector<SOCKET> connected_clients;
std::mutex clients_mutex;
std::atomic<bool> server_running{true};
std::unordered_map<SOCKET, std::string> client_names;
std::unordered_map<SOCKET, bool> client_logged_in;


struct ClientTask {
    SOCKET client_socket;
    int client_id;
};
std::queue<ClientTask> client_tasks;
std::mutex task_mutex;
std::condition_variable task_cv;
bool worker_pool_stopping = false;


void log_message(const std::string& text) {
    std::lock_guard<std::mutex> lock(output_mutex);

    std::cout << text << '\n';
}

void log_error(const std::string& text) {
    std::lock_guard<std::mutex> lock(output_mutex);

    std::cerr << text << '\n';
}

void add_client_task(
    SOCKET client_socket,
    int client_id
) {
    {
        std::lock_guard<std::mutex> lock(
            task_mutex
        );

        client_tasks.push({
            client_socket,
            client_id
        });
    }

    task_cv.notify_one();
}

void stop_worker_pool() {
    {
        std::lock_guard<std::mutex> lock(
            task_mutex
        );

        worker_pool_stopping = true;
    }

    task_cv.notify_all();
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

void send_to_client(
    SOCKET client_socket,
    const std::string& message
) {
    std::string packet = message + '\n';

    bool send_success = send_all(
        client_socket,
        packet.c_str(),
        static_cast<int>(packet.size())
    );

    if (!send_success) {
        log_error(
            "Send to client failed: " +
            std::to_string(WSAGetLastError())
        );
    }
}

enum class ReceiveStatus{
    success,
    disconnected,
    server_stopping,
    error
};

ReceiveStatus receive_line(
    SOCKET socket_handle,
    std::string& pending_data,
    std::string& message
) {
    while (true) {
        // 服务器准备关闭，当前客户端线程也结束
        if (!server_running) {
            return ReceiveStatus::server_stopping;
        }

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

        // 最多等待200毫秒，检查socket是否有数据
        fd_set read_sockets;
        FD_ZERO(&read_sockets);
        FD_SET(socket_handle, &read_sockets);

        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = 200000;

        int ready_count = select(
            0,
            &read_sockets,
            nullptr,
            nullptr,
            &timeout
        );

        if (ready_count == SOCKET_ERROR) {
            if (!server_running) {
                return ReceiveStatus::server_stopping;
            }

            return ReceiveStatus::error;
        }

        // 没有数据，重新回到循环顶部检查server_running
        if (ready_count == 0) {
            continue;
        }

        if (!FD_ISSET(socket_handle, &read_sockets)) {
            continue;
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
            if (!server_running) {
                return ReceiveStatus::server_stopping;
            }

            return ReceiveStatus::error;
        }

        pending_data.append(
            buffer,
            bytes_received
        );
    }
}

void add_client(
    SOCKET client_socket,
    int client_id
) {
    std::lock_guard<std::mutex> lock(
        clients_mutex
    );

    connected_clients.push_back(
        client_socket
    );

    client_names[client_socket] =
        "Client " +
        std::to_string(client_id);

    client_logged_in[client_socket] = false;
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

    client_names.erase(client_socket);
    client_logged_in.erase(client_socket);
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

std::string get_client_name(SOCKET client_socket) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    auto name_position = client_names.find(client_socket);
    if (name_position == client_names.end()) {
        return "Unknown";
    }
    return name_position->second;
}

bool change_client_name(
    SOCKET client_socket,
    const std::string& new_name,
    std::string& old_name
){
    std::lock_guard<std::mutex> lock(clients_mutex);

    for(const auto& name_entry : client_names){
        if(name_entry.first != client_socket &&
            name_entry.second == new_name){
                return false;
        }
    }
    auto name_position = client_names.find(client_socket);
        if(name_position == client_names.end()){
            return false;
        }
    old_name = name_position -> second;
    name_position ->second = new_name;
    client_logged_in[client_socket] = true;

    return true;
}

bool is_client_logged_in(
    SOCKET client_socket
) {
    std::lock_guard<std::mutex> lock(
        clients_mutex
    );

    auto login_position =
        client_logged_in.find(client_socket);

    if (login_position ==
        client_logged_in.end()) {
        return false;
    }

    return login_position->second;
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

        if (receive_status == ReceiveStatus::server_stopping) {
            break;
        }

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

        if (message == "/name") {
            send_to_client(
                client_socket,
                "[Server] Usage: /name <username>"
            );

            continue;
        }



        const std::string name_command = "/name ";

        if (message.rfind(name_command, 0) == 0) {
            std::string new_name = message.substr(
                name_command.size()
            );

            if (new_name.empty()) {
                send_to_client(
                    client_socket,
                    "[Server] Username cannot be empty."
                );

                continue;
            }

            if (new_name.size() > 16) {
                send_to_client(
                    client_socket,
                    "[Server] Username cannot exceed 16 characters."
                );

                continue;
            }

            if (new_name.find(' ') != std::string::npos) {
                send_to_client(
                    client_socket,
                    "[Server] Username cannot contain spaces."
                );

                continue;
            }

            std::string old_name;

            bool change_success = change_client_name(
                client_socket,
                new_name,
                old_name
            );

            if (!change_success) {
                send_to_client(
                    client_socket,
                    "[Server] Username is already in use."
                );

                continue;
            }

        std::string rename_message =
                "[Server] " +
                old_name +
                " is now known as " +
                new_name +
                ".";
        client_logged_in[client_socket] = true;

        log_message(rename_message);
        broadcast_message(rename_message);


            continue;
    }

    if (!is_client_logged_in(client_socket)) {
            send_to_client(
        client_socket,
        "[Server] Please set your username first."
        );

        continue;
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
    std::string client_name = get_client_name(client_socket);

    std::string chat_message =
        "[" +
        client_name +
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

void worker_thread() {
    while (true) {
        ClientTask task{};

        {
            std::unique_lock<std::mutex> lock(
                task_mutex
            );

            while (
                client_tasks.empty() &&
                !worker_pool_stopping
            ) {
                task_cv.wait(lock);
            }

            if (
                worker_pool_stopping &&
                client_tasks.empty()
            ) {
                return;
            }

            task = client_tasks.front();
            client_tasks.pop();
        }

        handle_client(
            task.client_socket,
            task.client_id
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
    //监听
    
    constexpr int worker_count = 2;
    std::vector<std::thread> worker_threads;
    for(int worker_id = 1;
        worker_id <= worker_count;
        ++worker_id){
        worker_threads.emplace_back(worker_thread);
        log_message(
            "Worker " +
            std::to_string(worker_id) +
            " started."
        );
    }


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

        add_client(client_socket,client_id);
        send_to_client(client_socket,
        "[Server] Please set your username with /name <username>");



        log_message(
            "Client " +
            std::to_string(client_id) +
            " connected. Online clients: " +
            std::to_string(get_client_count())
        );

        add_client_task(client_socket, client_id);
}
    // main 线程不再使用监听套接字，可以安全关闭
    closesocket(server_socket);

    // 让所有阻塞在 recv() 的客户端线程退出
    shutdown_all_clients();

    stop_worker_pool();

    for(std::thread& worker : worker_threads){
        if(worker.joinable()){
            worker.join();
        }
    }

    WSACleanup();

    log_message("Server stopped.");
    return 0;
}