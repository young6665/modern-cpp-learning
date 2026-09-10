#include <doctest/doctest.h>
#include <minichat/message_store.h>

TEST_CASE("MessageStore starts empty") {
    const minichat::MessageStore store;

    CHECK(store.size() == 0);
}

TEST_CASE("MessageStore accepts a normal message") {
    minichat::MessageStore store;

    minichat::Message message;
    message.sender = "young";
    message.content = "hello";

    CHECK(store.add(message));
    CHECK(store.size() == 1);
}

TEST_CASE("MessageStore rejects an empty message") {
    minichat::MessageStore store;

    minichat::Message message;
    message.sender = "young";
    message.content = "";

    CHECK_FALSE(store.add(message));
    CHECK(store.size() == 0);
}