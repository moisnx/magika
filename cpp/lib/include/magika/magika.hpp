#pragma once
#include "types.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace magika {

class Magika {
public:
  // Constructor: provide path to model directory
  // If empty, uses default bundled model
  explicit Magika(const std::string &model_dir = "");

  // Destructor
  ~Magika();

  // No copy (contains unique resources)
  Magika(const Magika &) = delete;
  Magika &operator=(const Magika &) = delete;

  // Move is OK
  Magika(Magika &&) noexcept;
  Magika &operator=(Magika &&) noexcept;

  // Identify a file by path
  MagikaResult identify_path(const std::string &filepath);

  // Identify raw bytes
  MagikaResult identify_bytes(const std::vector<uint8_t> &content);

  // Identify multiple files (batch)
  std::vector<MagikaResult>
  identify_paths(const std::vector<std::string> &paths);

  // Get list of all supported content types
  std::vector<std::string> get_output_content_types() const;

  // Get model version
  std::string get_model_name() const;

private:
  class Impl; // PIMPL pattern to hide ONNX details
  std::unique_ptr<Impl> impl_;
};

} // namespace magika