#include <minichat/message_store.h>
#include <minichat/command.h>

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
        const minichat::Command command = 
            minichat::parse_command(message.content);

        if (command == minichat::Command::quit) {
            break;
        }

        if (command == minichat::Command::list){
            store.print_all();
            continue;
        }

        if (command == minichat::Command::count) {
            std::cout << "Message count: "
                      << store.size()
                      << '\n';
            continue;
        }

        if (command == minichat::Command::clear){
            store.clear();
            std::cout << "All message cleared.\n";
            continue;
        }

        if (command == minichat::Command::help) {
            std::cout << "Commands:\n"
              << "  /list  - Show all messages\n"
              << "  /count - Show message count\n"
              << "  /clear - Clear all messages\n"
              << "  /quit  - Exit\n";
             continue;        

        if (!store.add(message)) {
            std::cout << "Message cannot be empty.\n";
            continue;
        }
}


    }

    store.print_all();

    std::cout << "\nPress Enter to exit...";
    std::cin.get();

    return 0;
}