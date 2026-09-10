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

TEST_CASE("MessageStore rejects a whitespace-only message") {
    // Arrange
    minichat::MessageStore store;

    minichat::Message message;
    message.sender = "young";
    message.content = "   ";

    // Act
    const bool added = store.add(message);

    // Assert
    CHECK_FALSE(added);
    CHECK(store.size() == 0);
}

TEST_CASE("MessageStore clears all messages") {
    minichat::MessageStore store;

    minichat::Message first_message;
    first_message.sender = "young";
    first_message.content = "1";

    minichat::Message second_message;
    second_message.sender = "young";
    second_message.content = "2";

    REQUIRE(store.add(first_message));
    REQUIRE(store.add(second_message));
    REQUIRE(store.size() == 2);

    store.clear();

    CHECK(store.size() == 0);
}