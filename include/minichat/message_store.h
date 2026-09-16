#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace minichat {

struct Message {
    std::string sender;
    std::string content;
};

class MessageStore {
public:
    bool add(const Message& message);
    std::size_t size() const;
    void print_all() const;
    void clear();

private:
    std::vector<Message> messages_;
};

}