#include <minichat/message_store.h>

#include <iostream>

namespace minichat {

bool MessageStore::add(const Message& message) {
    if (message.content.find_first_not_of(" \t\r\n")
        == std::string::npos) {
    return false;
}

    messages_.push_back(message);
    return true;
}

std::size_t MessageStore::size() const {
    return messages_.size();
}

void MessageStore::print_all() const {
    for (const Message& message : messages_) {
        std::cout << message.sender
                  << ": "
                  << message.content
                  << '\n';
    }
}

}