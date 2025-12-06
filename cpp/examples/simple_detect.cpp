#include <iostream>
#include <magika/magika.hpp>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <file>\n";
    return 1;
  }

  try {
    // Initialize with default model
    magika::Magika detector;

    // Detect file type
    auto result = detector.identify_path(argv[1]);

    std::cout << "File: " << result.path << "\n"
              << "Type: " << result.content_type << "\n"
              << "MIME: " << result.mime_type << "\n"
              << "Score: " << result.score << "\n";

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}