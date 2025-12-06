#include "magika/magika.hpp"
#include "vendor/nlohmann/json.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <onnxruntime_cxx_api.h>

namespace magika {

// Internal config structure

struct ContentTypeInfo {
  std::string label;
  std::string mime_type;
  std::string group;
  std::string description;
  bool is_text;
};

std::unordered_map<std::string, ContentTypeInfo> content_types_kb_;

// Add method to load it
void load_content_types_kb(const std::string &kb_path) {
  std::ifstream file(kb_path);
  if (!file) {
    throw std::runtime_error("Cannot open content types KB: " + kb_path);
  }

  nlohmann::json j;
  file >> j;

  for (auto &[label, info] : j.items()) {
    ContentTypeInfo ct_info;
    ct_info.label = label;
    ct_info.mime_type = info["mime_type"].is_null()
                            ? "application/octet-stream"
                            : info["mime_type"].get<std::string>();
    ct_info.group =
        info["group"].is_null() ? "unknown" : info["group"].get<std::string>();
    ct_info.description = info["description"].is_null()
                              ? label
                              : info["description"].get<std::string>();
    ct_info.is_text = info["is_text"];

    content_types_kb_[label] = ct_info;
  }
}

struct ModelConfig {
  int beg_size;
  int mid_size;
  int end_size;
  int padding_token;
  int block_size;
  int min_file_size_for_dl;
  std::vector<std::string> target_labels_space;

  static ModelConfig load(const std::string &config_path);
};

// Private implementation (hides ONNX from public headers)
class Magika::Impl {
public:
  Impl(const std::string &model_dir);

  MagikaResult identify_path(const std::string &filepath);
  MagikaResult identify_bytes(const std::vector<uint8_t> &content);

  std::string get_model_name() const { return model_name_; }
  const std::vector<std::string> &get_labels() const {
    return config_.target_labels_space;
  }

private:
  std::string model_dir_;
  std::string model_name_;
  ModelConfig config_;

  Ort::Env env_;
  std::unique_ptr<Ort::Session> session_;

  std::vector<int32_t> extract_features(const std::vector<uint8_t> &data);
  std::vector<float> run_inference(const std::vector<int32_t> &features);
};

// Load config implementation
ModelConfig ModelConfig::load(const std::string &config_path) {
  std::ifstream file(config_path);
  if (!file) {
    throw std::runtime_error("Cannot open config: " + config_path);
  }

  nlohmann::json j;
  file >> j;

  ModelConfig config;
  config.beg_size = j["beg_size"];
  config.mid_size = j["mid_size"];
  config.end_size = j["end_size"];
  config.padding_token = j["padding_token"];
  config.block_size = j["block_size"];
  config.min_file_size_for_dl = j["min_file_size_for_dl"];
  config.target_labels_space =
      j["target_labels_space"].get<std::vector<std::string>>();

  return config;
}

// Impl constructor
// Impl constructor
Magika::Impl::Impl(const std::string &model_dir)
    : model_dir_(model_dir), // Don't provide default!
      env_(ORT_LOGGING_LEVEL_WARNING, "magika") {

  if (model_dir_.empty()) {
    throw std::runtime_error(
        "Model directory must be provided. "
        "Example: Magika(\"/usr/local/share/magika/models/standard_v3_3\")");
  }

  // Load config
  std::string config_path = model_dir_ + "/config.min.json";
  config_ = ModelConfig::load(config_path);

  // Load mimetypes - use parent directory of model_dir
  // If model_dir is "/path/to/models/standard_v3_3"
  // Then kb is at "/path/to/content_types_kb.min.json"
  std::filesystem::path model_path(model_dir_);
  std::filesystem::path kb_path =
      model_path.parent_path().parent_path() / "content_types_kb.min.json";

  // Fallback: try same directory as model
  if (!std::filesystem::exists(kb_path)) {
    kb_path = model_path.parent_path() / "content_types_kb.min.json";
  }

  load_content_types_kb(kb_path.string());

  // Load ONNX model
  std::string model_onnx_path = model_dir_ + "/model.onnx";
  Ort::SessionOptions session_options;
  session_ = std::make_unique<Ort::Session>(env_, model_onnx_path.c_str(),
                                            session_options);

  // Extract model name from directory
  size_t last_slash = model_dir_.find_last_of("/\\");
  model_name_ = (last_slash != std::string::npos)
                    ? model_dir_.substr(last_slash + 1)
                    : model_dir_;
}

std::vector<int32_t>
Magika::Impl::extract_features(const std::vector<uint8_t> &data) {
  std::vector<int32_t> features;
  features.reserve(config_.beg_size + config_.mid_size + config_.end_size);

  int bytes_to_read =
      std::min(static_cast<int>(data.size()), config_.block_size);

  // === BEGINNING FEATURES ===
  std::vector<uint8_t> beg_content(data.begin(), data.begin() + bytes_to_read);

  // lstrip: remove whitespace from beginning
  auto beg_start = std::find_if(beg_content.begin(), beg_content.end(),
                                [](uint8_t c) { return !std::isspace(c); });
  beg_content.erase(beg_content.begin(), beg_start);

  // Take first beg_size bytes
  for (int i = 0; i < config_.beg_size && i < beg_content.size(); i++) {
    features.push_back(static_cast<int32_t>(beg_content[i]));
  }
  while (features.size() < config_.beg_size) {
    features.push_back(config_.padding_token);
  }

  // === MIDDLE FEATURES ===
  for (int i = 0; i < config_.mid_size; i++) {
    features.push_back(config_.padding_token);
  }

  // === END FEATURES ===
  int end_start = std::max(0, static_cast<int>(data.size()) - bytes_to_read);
  std::vector<uint8_t> end_content(data.begin() + end_start, data.end());

  // rstrip: remove whitespace from end
  auto end_last = std::find_if(end_content.rbegin(), end_content.rend(),
                               [](uint8_t c) { return !std::isspace(c); });
  end_content.erase(end_last.base(), end_content.end());

  // Take last end_size bytes
  int end_offset =
      std::max(0, static_cast<int>(end_content.size()) - config_.end_size);
  for (int i = end_offset; i < end_content.size(); i++) {
    features.push_back(static_cast<int32_t>(end_content[i]));
  }
  while (features.size() <
         (config_.beg_size + config_.mid_size + config_.end_size)) {
    features.insert(features.begin() + config_.beg_size + config_.mid_size,
                    config_.padding_token);
  }

  return features;
}

std::vector<float>
Magika::Impl::run_inference(const std::vector<int32_t> &features) {
  auto memory_info =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

  std::vector<int64_t> input_shape = {1, static_cast<int64_t>(features.size())};

  auto input_tensor = Ort::Value::CreateTensor<int32_t>(
      memory_info, const_cast<int32_t *>(features.data()), features.size(),
      input_shape.data(), input_shape.size());

  const char *input_names[] = {"bytes"};
  const char *output_names[] = {"target_label"};

  auto output_tensors = session_->Run(Ort::RunOptions{nullptr}, input_names,
                                      &input_tensor, 1, output_names, 1);

  float *output_data = output_tensors[0].GetTensorMutableData<float>();
  auto output_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();

  size_t output_size = output_shape[1];
  std::vector<float> predictions(output_data, output_data + output_size);

  return predictions;
}

MagikaResult Magika::Impl::identify_bytes(const std::vector<uint8_t> &content) {
  auto features = extract_features(content);
  auto predictions = run_inference(features);

  auto max_it = std::max_element(predictions.begin(), predictions.end());
  int predicted_class = std::distance(predictions.begin(), max_it);
  float confidence = *max_it;

  MagikaResult result;
  result.path = "-";
  result.content_type = config_.target_labels_space[predicted_class];
  result.score = confidence;

  // Add MIME type and group
  if (content_types_kb_.count(result.content_type)) {
    auto &info = content_types_kb_[result.content_type];
    result.mime_type = info.mime_type;
    result.group = info.group;
  } else {
    result.mime_type = "application/octet-stream";
    result.group = "unknown";
  }
  return result;
}

MagikaResult Magika::Impl::identify_path(const std::string &filepath) {
  // Read file
  std::ifstream file(filepath, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Cannot open file: " + filepath);
  }

  // FIX: Add extra parentheses or use brace initialization
  std::vector<uint8_t> content{std::istreambuf_iterator<char>(file),
                               std::istreambuf_iterator<char>()};

  auto result = identify_bytes(content);
  result.path = filepath;
  return result;
}

// Public API implementation (delegates to Impl)
Magika::Magika(const std::string &model_dir)
    : impl_(std::make_unique<Impl>(model_dir)) {}

Magika::~Magika() = default;
Magika::Magika(Magika &&) noexcept = default;
Magika &Magika::operator=(Magika &&) noexcept = default;

MagikaResult Magika::identify_path(const std::string &filepath) {
  return impl_->identify_path(filepath);
}

MagikaResult Magika::identify_bytes(const std::vector<uint8_t> &content) {
  return impl_->identify_bytes(content);
}

std::vector<MagikaResult>
Magika::identify_paths(const std::vector<std::string> &paths) {
  std::vector<MagikaResult> results;
  results.reserve(paths.size());
  for (const auto &path : paths) {
    results.push_back(identify_path(path));
  }
  return results;
}

std::vector<std::string> Magika::get_output_content_types() const {
  return impl_->get_labels();
}

std::string Magika::get_model_name() const { return impl_->get_model_name(); }

} // namespace magika