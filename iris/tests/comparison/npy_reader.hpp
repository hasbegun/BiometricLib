#pragma once

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

namespace iris::npy {

/// Minimal .npy reader for cross-language comparison tests.
/// Supports 1D/2D/3D arrays of float32, float64, bool, uint8, int32, int64.
///
/// Format spec: https://numpy.org/devdocs/reference/generated/numpy.lib.format.html

struct NpyArray {
    std::vector<uint8_t> data;
    std::vector<size_t> shape;
    size_t element_size = 0;
    char dtype = 0;  // 'f' = float, 'b' = bool, 'u' = unsigned int, 'i' = signed int

    /// Total number of elements.
    [[nodiscard]] size_t num_elements() const {
        size_t n = 1;
        for (auto s : shape) n *= s;
        return n;
    }

    /// Read as cv::Mat (for 2D arrays). Returns CV_64FC1 for float64, etc.
    [[nodiscard]] cv::Mat to_mat() const {
        if (shape.size() < 2) {
            throw std::runtime_error("to_mat() requires at least 2D array");
        }

        int rows = static_cast<int>(shape[0]);
        int cols = static_cast<int>(shape[1]);

        // If 3D, fold last dimension into columns
        if (shape.size() == 3) {
            cols *= static_cast<int>(shape[2]);
        }

        int cv_type = CV_8UC1;
        if (dtype == 'f' && element_size == 8) cv_type = CV_64FC1;
        else if (dtype == 'f' && element_size == 4) cv_type = CV_32FC1;
        else if (dtype == 'b' || (dtype == 'u' && element_size == 1)) cv_type = CV_8UC1;
        else if (dtype == 'i' && element_size == 4) cv_type = CV_32SC1;

        cv::Mat mat(rows, cols, cv_type);
        std::memcpy(mat.data, data.data(), data.size());
        return mat;
    }
};

/// Read a .npy file from disk.
inline NpyArray read_npy(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open .npy file: " + path.string());
    }

    // Magic: \x93NUMPY
    char magic[6];
    file.read(magic, 6);
    if (magic[0] != '\x93' || std::string(magic + 1, 5) != "NUMPY") {
        throw std::runtime_error("Invalid .npy magic bytes: " + path.string());
    }

    // Version
    uint8_t major_ver = 0, minor_ver = 0;
    file.read(reinterpret_cast<char*>(&major_ver), 1);
    file.read(reinterpret_cast<char*>(&minor_ver), 1);

    // Header length
    uint32_t header_len = 0;
    if (major_ver == 1) {
        uint16_t hl16 = 0;
        file.read(reinterpret_cast<char*>(&hl16), 2);
        header_len = hl16;
    } else {
        file.read(reinterpret_cast<char*>(&header_len), 4);
    }

    // Read header string (Python dict-like)
    std::string header(header_len, '\0');
    file.read(header.data(), static_cast<std::streamsize>(header_len));

    NpyArray result;

    // Parse dtype: look for 'descr': '<f8', '<f4', '|b1', '<u1', etc.
    auto descr_pos = header.find("'descr'");
    if (descr_pos == std::string::npos) descr_pos = header.find("\"descr\"");
    if (descr_pos != std::string::npos) {
        auto quote1 = header.find('\'', descr_pos + 7);
        if (quote1 == std::string::npos) quote1 = header.find('"', descr_pos + 7);
        auto quote2 = header.find_first_of("'\"", quote1 + 1);
        auto descr = header.substr(quote1 + 1, quote2 - quote1 - 1);

        // Parse: endian(1 char) + type(1 char) + size(digits)
        if (descr.size() >= 3) {
            result.dtype = descr[1];
            result.element_size = static_cast<size_t>(std::stoi(descr.substr(2)));
        } else if (descr == "|b1") {
            result.dtype = 'b';
            result.element_size = 1;
        }
    }

    // Parse shape: look for 'shape': (N, M, ...)
    auto shape_pos = header.find("'shape'");
    if (shape_pos == std::string::npos) shape_pos = header.find("\"shape\"");
    if (shape_pos != std::string::npos) {
        auto paren_open = header.find('(', shape_pos);
        auto paren_close = header.find(')', paren_open);
        auto shape_str = header.substr(paren_open + 1, paren_close - paren_open - 1);

        // Parse comma-separated integers
        size_t pos = 0;
        while (pos < shape_str.size()) {
            while (pos < shape_str.size() && (shape_str[pos] == ' ' || shape_str[pos] == ','))
                ++pos;
            if (pos >= shape_str.size()) break;
            size_t end = shape_str.find_first_of(", )", pos);
            if (end == std::string::npos) end = shape_str.size();
            auto dim_str = shape_str.substr(pos, end - pos);
            if (!dim_str.empty()) {
                result.shape.push_back(static_cast<size_t>(std::stoul(dim_str)));
            }
            pos = end + 1;
        }
    }

    // Read data
    size_t data_size = result.num_elements() * result.element_size;
    result.data.resize(data_size);
    file.read(reinterpret_cast<char*>(result.data.data()),
              static_cast<std::streamsize>(data_size));

    return result;
}

}  // namespace iris::npy
