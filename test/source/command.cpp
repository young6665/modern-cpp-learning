#include <doctest/doctest.h>
#include <minichat/command.h>

TEST_CASE("parse_command recognizes chat commands") {
    CHECK(minichat::parse_command("/quit")
          == minichat::Command::quit);

    CHECK(minichat::parse_command("/list")
          == minichat::Command::list);

    CHECK(minichat::parse_command("/count")
          == minichat::Command::count);

    CHECK(minichat::parse_command("/clear")
          == minichat::Command::clear);

    CHECK(minichat::parse_command("/help")
          == minichat::Command::help);

}

TEST_CASE("parse_command treats normal text as none") {
    CHECK(minichat::parse_command("hello")
          == minichat::Command::none);
}