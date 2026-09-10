#include <minichat/message_store.h>

#include <iostream>
#include <string>

minichat::Message read_message(const std::string& sender) {
    minichat::Message msg;
    msg.sender = sender;

    std::cout << "Message: ";
    std::getline(std::cin, msg.content);

    return msg;
}

int main() {
    minichat::MessageStore store;
    std::string sender;

    std::cout << "Sender: ";
    std::getline(std::cin, sender);

    while (true) {
        minichat::Message message = read_message(sender);

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

        if(message.content == "/clear"){
            store.clear();
            std::cout << "All message cleared.\n";
            continue;
        }

        if (!store.add(message)) {
            std::cout << "Message cannot be empty.\n";
            continue;
        }
    }

    store.print_all();

    std::cout << "\nPress Enter to exit...";
    std::cin.get();

    return 0;
}