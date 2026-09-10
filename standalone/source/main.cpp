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


void print_message(const Message& message){
    std::cout << message.sender
              << ": "
              << message.content
              << '\n';

}


class MessageStore {
public:
    bool add(const Message& message) {
        if (message.content.empty()) {
            return false;
        }

    messages_.push_back(message);
    return true;
}

    std::size_t size() const {
        return messages_.size();
    }

    void print_all() const {
        for (const Message& message : messages_) {
            print_message(message);
        }
    }

private:
    std::vector<Message> messages_;
};

int main() {
    MessageStore store;
    std::string sender;
    std::cout << "Sender: " ;
    std::getline(std::cin , sender);


while (true) {
    Message message = read_message(sender);

    if (message.content == "/quit") {
        break;
    }

    if (message.content == "/list") {
        store.print_all();
        continue;
    }

    if (message.content == "/count") {
        std::cout << "Message count: "
                  << store.size()
                  << '\n';
        continue;
    }

    if (!store.add(message)) {
        std::cout << "Message cannot be empty.\n";
        continue;
    }
}


    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    return 0;
}