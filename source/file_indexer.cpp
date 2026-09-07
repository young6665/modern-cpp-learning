#include <indexer/file_indexer.h>

#include <algorithm>
#include <stdexcept>

namespace indexer {

  std::vector<FileRecord> FileIndexer::index(const std::filesystem::path& root) const {
    namespace fs = std::filesystem;

    if (!fs::exists(root)) {
      throw std::invalid_argument("root path does not exist");
    }

    if (!fs::is_directory(root)) {
      throw std::invalid_argument("root path is not a directory");
    }

    std::vector<FileRecord> records;
    const auto options = fs::directory_options::skip_permission_denied;

    for (const auto& entry : fs::recursive_directory_iterator(root, options)) {
      if (entry.is_regular_file()) {
        records.push_back({entry.path(), entry.file_size()});
      }
    }

    std::sort(records.begin(), records.end(), [](const FileRecord& left, const FileRecord& right) {
      return left.path < right.path;
    });

    return records;
  }

}  // namespace indexer