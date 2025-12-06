#pragma once
#include <string>
#include <vector>

namespace magika {

struct MagikaResult {
  std::string path;
  std::string content_type; // e.g., "python", "javascript"
  std::string mime_type;    // e.g., "text/x-python"
  std::string group;        // e.g., "code", "text"
  float score;              // confidence 0.0-1.0

  // For JSON output
  std::string to_json() const;
};

enum class PredictionMode { BEST_GUESS, MEDIUM_CONFIDENCE, HIGH_CONFIDENCE };

} // namespace magika