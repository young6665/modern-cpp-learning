#include <iostream>
#include <string>
#include <vector>
#include <cstddef>

struct Message {
    std::string sender;
    std::string content;
};

int main() {
    std::vector<Message> messages;
    Message msg;

for (int i = 0; i <= 3; ++i) {
    Message msg;

    std::cout << "Sender: ";
    std::getline(std::cin, msg.sender);

    std::cout << "Message: ";
    std::getline(std::cin, msg.content);

    messages.push_back(msg);
}

    std::cout << "Message count: "
              << messages.size()
              << '\n';

for (std::size_t i = 0; i < messages.size(); ++i) {
    std::cout << messages[i].sender
              << ": "
              << messages[i].content
              << '\n';
}
    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    return 0;
}