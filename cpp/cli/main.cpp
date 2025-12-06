#include <filesystem>
#include <iostream>
#include <magika/magika.hpp>
#include <vector>

namespace fs = std::filesystem;

void print_usage(const char *prog_name) {
  std::cout << "Usage: " << prog_name << " [OPTIONS] FILE [FILE...]\n\n"
            << "Options:\n"
            << "  -h, --help           Show this help message\n"
            << "  --json               Output in JSON format\n"
            << "  --label              Output only the label\n"
            << "  --mime-type          Output only the MIME type\n"
            << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  bool json_output = false;
  bool label_only = false;
  bool mime_only = false;
  std::vector<std::string> files;

  // Parse arguments
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      print_usage(argv[0]);
      return 0;
    } else if (arg == "--json") {
      json_output = true;
    } else if (arg == "--label") {
      label_only = true;
    } else if (arg == "--mime-type") {
      mime_only = true;
    } else {
      files.push_back(arg);
    }
  }

  if (files.empty()) {
    std::cerr << "Error: No files specified\n";
    print_usage(argv[0]);
    return 1;
  }

  try {
    // Initialize Magika with default model
    magika::Magika detector;

    for (const auto &filepath : files) {
      auto result = detector.identify_path(filepath);

      if (json_output) {
        std::cout << "{\n"
                  << "  \"path\": \"" << result.path << "\",\n"
                  << "  \"content_type\": \"" << result.content_type << "\",\n"
                  << "  \"score\": " << result.score << "\n"
                  << "}\n";
      } else if (label_only) {
        std::cout << result.content_type << "\n";
      } else if (mime_only) {
        std::cout << result.mime_type << "\n";
      } else {
        // Default output (similar to official magika CLI)
        std::cout << result.path << ": " << result.content_type
                  << " (score: " << result.score << ")\n";
      }
    }

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}