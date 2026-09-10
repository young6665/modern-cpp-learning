#include <iostream>
#include <string>
#include <vector>
#include <cstddef>

struct Message {
    std::string sender;
    std::string content;
};

Message read_message(const std::string& sender){
    Message msg;
    msg.sender = sender;

    std::cout << "Message: ";
    std::getline(std::cin, msg.content);
    return msg;
}

void print_message_size(const std::vector<Message>& messages){
    int messages_size = messages.size();
    std::cout << "Messages count : " 
              << messages_size
              << '\n';

}

void print_message(const Message& message){
    std::cout << message.sender
              << ": "
              << message.content
              << '\n';

}

void print_messages(const std::vector<Message>& messages) {

    for (const Message& message : messages) {
        print_message(message);
    }
}

int main() {
    std::vector<Message> messages;
    std::string sender;
    std::cout << "Sender: " ;
    std::getline(std::cin , sender);


while (true) {
    Message message = read_message(sender);

    if (message.content == "/quit") {
        break;
    }

    if (message.content == "/list") {
        print_messages(messages);
        continue;
    }

    if (message.content == "/count") {
        print_message_size(messages);
        continue;
    }

    messages.push_back(message);
}


print_messages(messages);
    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    return 0;
}