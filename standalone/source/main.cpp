#include <iostream>
#include <string>

struct Message {
    std::string sender;
    std::string content;
};

int main() {
    Message msg;
    std::cout << "sender: ";
    std::getline(std::cin, msg.sender);
    std::cout << "content: ";
    std::getline(std::cin, msg.content);
    std::cout << msg.sender << ": " << msg.content << std::endl;
    system("pause");
    return 0;
}