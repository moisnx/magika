#include "termcolor.hpp"
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <magika/magika.hpp>
#include <numeric>
#include <vector>

namespace fs = std::filesystem;

struct Options {
  bool json_output = false;
  bool jsonl_output = false;
  bool label_only = false;
  bool mime_only = false;
  bool show_score = false;
  bool recursive = false;
  bool no_dereference = false;
  bool colors = true; // Default to true
  std::vector<std::string> paths;
};

std::string find_magika_models() {
  // Try environment variable first
  if (const char *env_path = std::getenv("MAGIKA_MODEL_PATH")) {
    if (fs::exists(env_path)) {
      return env_path;
    }
  }

  // Try common install locations
  std::vector<std::string> search_paths = {
      "/usr/local/share/magika/models/standard_v3_3",
      "/usr/share/magika/models/standard_v3_3",
      "../../assets/models/standard_v3_3", // Development build
      "./models/standard_v3_3",            // Bundled
      "../models/standard_v3_3"            // Build directory
  };

  for (const auto &path : search_paths) {
    if (fs::exists(path + "/model.onnx")) {
      return path;
    }
  }

  throw std::runtime_error(
      "Magika models not found. Tried:\n" +
      std::accumulate(search_paths.begin(), search_paths.end(), std::string(),
                      [](const std::string &a, const std::string &b) {
                        return a + "  - " + b + "\n";
                      }) +
      "\nInstall with: sudo cmake --install build\n"
      "Or set MAGIKA_MODEL_PATH=/path/to/models/standard_v3_3");
}

void print_usage(const char *prog_name) {
  std::cout << "Determines file content types using AI\n\n"
            << "Usage: " << prog_name << " [OPTIONS] [PATH]...\n\n"
            << "Arguments:\n"
            << "  [PATH]...  List of paths to the files to analyze\n\n"
            << "Options:\n"
            << "  -r, --recursive        Identifies files within directories\n"
            << "      --model-dir PATH   Path to model directory\n"
            << "      --no-dereference   Identifies symbolic links as is\n"
            << "      --no-dereference   Identifies symbolic links as is\n"
            << "  -s, --output-score     Prints the prediction score\n"
            << "  -i, --mime-type        Prints the MIME type\n"
            << "  -l, --label            Prints a simple label\n"
            << "      --json             Prints in JSON format\n"
            << "      --jsonl            Prints in JSONL format\n"
            << "      --colors           Prints with colors\n"
            << "      --no-colors        Prints without colors\n"
            << "  -h, --help             Print help\n"
            << "  -V, --version          Print version\n"
            << std::endl;
}

void collect_files(const std::string &path, std::vector<std::string> &files,
                   bool recursive, bool no_dereference) {
  fs::path p(path);

  if (!fs::exists(p)) {
    std::cerr << termcolor::red << "Error: Path does not exist: " << path
              << termcolor::reset << std::endl;
    return;
  }

  if (fs::is_directory(p)) {
    if (recursive) {
      for (const auto &entry : fs::recursive_directory_iterator(p)) {
        if (fs::is_regular_file(entry)) {
          files.push_back(entry.path().string());
        }
      }
    } else {
      std::cerr << termcolor::red << "Error: " << path
                << " is a directory (use -r for recursive)\n"
                << termcolor::reset;
    }
  } else {
    files.push_back(path);
  }
}

std::string get_description(const magika::MagikaResult &result) {
  // TODO: Load descriptions from content_types_kb.min.json
  // For now, return formatted content type
  std::string desc = result.content_type;
  if (!result.group.empty() && result.group != "unknown") {
    desc += " (" + result.group + ")";
  }
  return desc;
}

std::ostream &get_group_color(std::ostream &os, const std::string &group) {
  if (group == "code")
    return os << termcolor::cyan;
  if (group == "text")
    return os << termcolor::green;
  if (group == "executable")
    return os << termcolor::red << termcolor::bold;
  if (group == "archive")
    return os << termcolor::magenta;
  if (group == "image")
    return os << termcolor::yellow;
  if (group == "video")
    return os << termcolor::blue;
  if (group == "audio")
    return os << termcolor::blue;
  if (group == "document")
    return os << termcolor::cyan;
  return os << termcolor::white;
}

void print_result_default(const magika::MagikaResult &result, bool show_score,
                          bool colors) {
  std::string description = get_description(result);
  const float HIGH_CONFIDENCE = 0.5;

  if (colors) {
    // Print path in bold
    std::cout << termcolor::bold << result.path << termcolor::reset << ": ";

    // Print description with group-based color
    get_group_color(std::cout, result.group) << description << termcolor::reset;

    // Show low-confidence warning
    if (result.score < HIGH_CONFIDENCE) {
      std::cout << " " << termcolor::yellow
                << "[Low-confidence model best-guess: " << result.content_type;
      if (show_score) {
        std::cout << ", score=" << std::fixed << std::setprecision(3)
                  << result.score;
      }
      std::cout << "]" << termcolor::reset;
    } else if (show_score) {
      std::cout << " " << termcolor::dark << "(score: " << std::fixed
                << std::setprecision(3) << result.score << ")"
                << termcolor::reset;
    }
  } else {
    // Original non-colored output
    std::cout << result.path << ": " << description;

    if (result.score < HIGH_CONFIDENCE) {
      std::cout << " [Low-confidence model best-guess: " << result.content_type;
      if (show_score) {
        std::cout << ", score=" << std::fixed << std::setprecision(3)
                  << result.score;
      }
      std::cout << "]";
    } else if (show_score) {
      std::cout << " (score: " << std::fixed << std::setprecision(3)
                << result.score << ")";
    }
  }

  std::cout << std::endl;
}

void print_result_json(const magika::MagikaResult &result) {
  std::cout << "{\n"
            << "  \"path\": \"" << result.path << "\",\n"
            << "  \"content_type\": \"" << result.content_type << "\",\n"
            << "  \"mime_type\": \"" << result.mime_type << "\",\n"
            << "  \"group\": \"" << result.group << "\",\n"
            << "  \"score\": " << result.score << "\n"
            << "}\n";
}

void print_result_jsonl(const magika::MagikaResult &result) {
  std::cout << "{\"path\":\"" << result.path << "\""
            << ",\"content_type\":\"" << result.content_type << "\""
            << ",\"mime_type\":\"" << result.mime_type << "\""
            << ",\"group\":\"" << result.group << "\""
            << ",\"score\":" << result.score << "}\n";
}

Options parse_args(int argc, char *argv[]) {
  Options opts;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      print_usage(argv[0]);
      exit(0);
    } else if (arg == "-V" || arg == "--version") {
      std::cout << "magika-cpp 1.0.0\n";
      exit(0);
    } else if (arg == "-r" || arg == "--recursive") {
      opts.recursive = true;
    } else if (arg == "--no-dereference") {
      opts.no_dereference = true;
    } else if (arg == "-s" || arg == "--output-score") {
      opts.show_score = true;
    } else if (arg == "-i" || arg == "--mime-type") {
      opts.mime_only = true;
    } else if (arg == "-l" || arg == "--label") {
      opts.label_only = true;
    } else if (arg == "--json") {
      opts.json_output = true;
    } else if (arg == "--jsonl") {
      opts.jsonl_output = true;
    } else if (arg == "--colors") {
      opts.colors = true;
    } else if (arg == "--no-colors") {
      opts.colors = false;
    } else if (arg[0] == '-') {
      std::cerr << termcolor::red << "Unknown option: " << arg
                << termcolor::reset << std::endl;
      exit(1);
    } else {
      opts.paths.push_back(arg);
    }
  }

  return opts;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  Options opts = parse_args(argc, argv);

  if (opts.paths.empty()) {
    std::cerr << "Error: No files specified\n";
    print_usage(argv[0]);
    return 1;
  }

  try {
    // Collect all files to process
    std::vector<std::string> files;
    for (const auto &path : opts.paths) {
      collect_files(path, files, opts.recursive, opts.no_dereference);
    }

    if (files.empty()) {
      std::cerr << "No files to process\n";
      return 1;
    }

    // Initialize Magika with auto-detected path
    std::string model_path = find_magika_models();
    magika::Magika detector(model_path);

    // Process files
    for (const auto &filepath : files) {
      try {
        auto result = detector.identify_path(filepath);

        if (opts.json_output) {
          print_result_json(result);
        } else if (opts.jsonl_output) {
          print_result_jsonl(result);
        } else if (opts.label_only) {
          std::cout << result.content_type << "\n";
        } else if (opts.mime_only) {
          std::cout << result.mime_type << "\n";
        } else {
          print_result_default(result, opts.show_score, opts.colors);
        }
      } catch (const std::exception &e) {
        std::cerr << "Error processing " << filepath << ": " << e.what()
                  << std::endl;
      }
    }

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}