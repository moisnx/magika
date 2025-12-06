# Magika C++ Binding

C++ implementation of Magika using ONNX Runtime for fast, accurate file type detection.

## Features

- 🚀 Fast inference (~5ms per file)
- 🎯 High accuracy using deep learning
- 📦 Clean C++ API
- 🔧 Easy integration into C++ projects
- 💻 Command-line tool included

## Building

### Prerequisites

- C++17 compatible compiler
- CMake 3.15+
- ONNX Runtime 1.16+

### Build Instructions
```bash
# Download ONNX Runtime
wget https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-linux-x64-1.16.3.tgz
tar -xzf onnxruntime-linux-x64-1.16.3.tgz

# Build Magika
cd magika/cpp
mkdir build && cd build
cmake -DONNXRUNTIME_DIR=/path/to/onnxruntime ..
cmake --build .

# Install (optional)
sudo cmake --install .
```

## Usage

### As a Library
```cpp
#include <magika/magika.hpp>

int main() {
    magika::Magika detector;
    auto result = detector.identify_path("test.py");
    
    std::cout << "Type: " << result.content_type << "\n";
    std::cout << "MIME: " << result.mime_type << "\n";
    std::cout << "Confidence: " << result.score << "\n";
    
    return 0;
}
```

### Command Line
```bash
# Detect single file
./magika file.py

# JSON output
./magika --json file.py

# Label only
./magika --label file.py

# Multiple files
./magika file1.py file2.js file3.cpp
```

## API Reference

### `Magika` Class
```cpp
namespace magika {

class Magika {
public:
    // Constructor - provide path to model directory
    explicit Magika(const std::string& model_dir = "");
    
    // Identify a file
    MagikaResult identify_path(const std::string& filepath);
    
    // Identify raw bytes
    MagikaResult identify_bytes(const std::vector<uint8_t>& content);
    
    // Batch identification
    std::vector<MagikaResult> identify_paths(const std::vector<std::string>& paths);
    
    // Get supported content types
    std::vector<std::string> get_output_content_types() const;
};

struct MagikaResult {
    std::string path;
    std::string content_type;  // e.g., "python"
    std::string mime_type;     // e.g., "text/x-python"
    std::string group;         // e.g., "code"
    float score;               // 0.0 - 1.0
};

}
```

## Examples

See the `examples/` directory for complete examples:

- `simple_detect.cpp` - Basic file detection
- `batch_detect.cpp` - Detecting multiple files

## Performance

- ~5ms inference time per file
- Scales to 1000+ files/second on modern CPUs
- Constant time regardless of file size

## License

Apache 2.0 - See LICENSE file

## Contributing

This is part of the official Magika project. See the main repository for contribution guidelines.