#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace indexer {

  struct FileRecord {
    std::filesystem::path path;
    std::uintmax_t size_bytes;
  };

  class FileIndexer {
  public:
    std::vector<FileRecord> index(const std::filesystem::path& root) const;
  };

}  // namespace indexer