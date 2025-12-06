#include <iostream>
#include <magika/magika.hpp>
#include <vector>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <file1> <file2> ...\n";
    return 1;
  }

  try {
    magika::Magika detector;

    std::vector<std::string> files;
    for (int i = 1; i < argc; i++) {
      files.push_back(argv[i]);
    }

    auto results = detector.identify_paths(files);

    for (const auto &result : results) {
      std::cout << result.path << ": " << result.content_type << " ("
                << result.score << ")\n";
    }

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}