#include <doctest/doctest.h>
#include <indexer/file_indexer.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>

TEST_CASE("FileIndexer scans files recursively") {
  namespace fs = std::filesystem;

  const auto root = fs::temp_directory_path() / "file_indexer_scan_test";

  fs::remove_all(root);
  fs::create_directories(root / "nested");

  std::ofstream(root / "a.txt") << "abc";
  std::ofstream(root / "nested" / "b.txt") << "12345";

  const indexer::FileIndexer file_indexer;
  const auto records = file_indexer.index(root);

  REQUIRE(records.size() == 2);

  CHECK(records.at(0).path.filename() == fs::path("a.txt"));
  CHECK(records.at(0).size_bytes == 3);
  CHECK(records.at(1).path.filename() == fs::path("b.txt"));
  CHECK(records.at(1).size_bytes == 5);

  fs::remove_all(root);
}

TEST_CASE("FileIndexer rejects a missing directory") {
  namespace fs = std::filesystem;

  const auto missing = fs::temp_directory_path() / "file_indexer_missing_test";
  fs::remove_all(missing);

  const indexer::FileIndexer file_indexer;

  CHECK_THROWS_AS(file_indexer.index(missing), std::invalid_argument);
}