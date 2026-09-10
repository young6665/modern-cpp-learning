#include <iostream>
#include <string>
#include <vector>
#include <cstddef>

struct Message {
    std::string sender;
    std::string content;
};

Message read_message(){
    Message msg;

    std::cout << "Sender: ";
    std::getline(std::cin , msg.sender);

    std::cout << "Message: ";
    std::getline(std::cin, msg.content);

    return msg;
}

void print_message(const Message& message){
    std::cout << message.sender
              << ": "
              << message.content
              << '\n';

}

int main() {
    std::vector<Message> messages;

for (int i = 0; i <= 3; ++i) {
   messages.push_back(read_message());
}

    std::cout << "Message count: "
              << messages.size()
              << '\n';

for (const Message& message : messages) {
    print_message(message);
}
    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    return 0;
}