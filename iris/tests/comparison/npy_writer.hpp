#pragma once

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "npy_reader.hpp"

namespace iris::npy {

/// Write a cv::Mat to .npy format (v1.0).
/// Supports: CV_32FC1 (<f4), CV_64FC1 (<f8), CV_8UC1 (<u1), CV_32SC1 (<i4).
inline void write_npy(const std::filesystem::path& path, const cv::Mat& mat,
                      const std::vector<size_t>& shape = {}) {
    std::string descr;
    switch (mat.type()) {
        case CV_32FC1: descr = "<f4"; break;
        case CV_64FC1: descr = "<f8"; break;
        case CV_8UC1:  descr = "<u1"; break;  // NOLINT
        case CV_32SC1: descr = "<i4"; break;
        default:
            throw std::runtime_error("Unsupported cv::Mat type for NPY: " +
                                     std::to_string(mat.type()));
    }

    // Build shape string
    std::string shape_str = "(";
    if (!shape.empty()) {
        for (size_t i = 0; i < shape.size(); ++i) {
            if (i > 0) shape_str += ", ";
            shape_str += std::to_string(shape[i]);
        }
        if (shape.size() == 1) shape_str += ",";  // tuple needs trailing comma
    } else {
        shape_str += std::to_string(static_cast<size_t>(mat.rows));
        shape_str += ", ";
        shape_str += std::to_string(static_cast<size_t>(mat.cols));
    }
    shape_str += ")";

    // Build header dict
    std::string header_dict = "{'descr': '" + descr +
                              "', 'fortran_order': False, 'shape': " +
                              shape_str + ", }";

    // Pad header to 64-byte alignment (magic=6 + ver=2 + header_len=2 + header)
    constexpr size_t kPreambleSize = 10;  // magic(6) + version(2) + header_len(2)
    size_t padding = 64 - ((kPreambleSize + header_dict.size() + 1) % 64);
    if (padding == 64) padding = 0;
    header_dict += std::string(padding, ' ');
    header_dict += '\n';

    auto header_len = static_cast<uint16_t>(header_dict.size());

    // Ensure continuous data
    cv::Mat continuous = mat.isContinuous() ? mat : mat.clone();

    // Write file
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open .npy file for writing: " +
                                 path.string());
    }

    // Magic: \x93NUMPY
    const char magic[] = "\x93NUMPY";
    file.write(magic, 6);

    // Version 1.0
    uint8_t ver_major = 1, ver_minor = 0;
    file.write(reinterpret_cast<const char*>(&ver_major), 1);
    file.write(reinterpret_cast<const char*>(&ver_minor), 1);

    // Header length (little-endian uint16)
    file.write(reinterpret_cast<const char*>(&header_len), 2);

    // Header dict
    file.write(header_dict.data(), static_cast<std::streamsize>(header_dict.size()));

    // Data
    size_t data_size = static_cast<size_t>(continuous.total()) * continuous.elemSize();
    file.write(reinterpret_cast<const char*>(continuous.data),
               static_cast<std::streamsize>(data_size));
}

/// Write an NpyArray to .npy format (v1.0).
inline void write_npy(const std::filesystem::path& path, const NpyArray& arr) {
    std::string descr;
    if (arr.dtype == 'f' && arr.element_size == 4) descr = "<f4";
    else if (arr.dtype == 'f' && arr.element_size == 8) descr = "<f8";
    else if (arr.dtype == 'u' && arr.element_size == 1) descr = "<u1";
    else if (arr.dtype == 'b' && arr.element_size == 1) descr = "|b1";
    else if (arr.dtype == 'i' && arr.element_size == 4) descr = "<i4";
    else if (arr.dtype == 'i' && arr.element_size == 8) descr = "<i8";
    else throw std::runtime_error("Unsupported dtype for NPY write");

    // Build shape string
    std::string shape_str = "(";
    for (size_t i = 0; i < arr.shape.size(); ++i) {
        if (i > 0) shape_str += ", ";
        shape_str += std::to_string(arr.shape[i]);
    }
    if (arr.shape.size() == 1) shape_str += ",";
    shape_str += ")";

    // Build header dict
    std::string header_dict = "{'descr': '" + descr +
                              "', 'fortran_order': False, 'shape': " +
                              shape_str + ", }";

    // Pad to 64-byte alignment
    constexpr size_t kPreambleSize = 10;
    size_t padding = 64 - ((kPreambleSize + header_dict.size() + 1) % 64);
    if (padding == 64) padding = 0;
    header_dict += std::string(padding, ' ');
    header_dict += '\n';

    auto header_len = static_cast<uint16_t>(header_dict.size());

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open .npy file for writing: " +
                                 path.string());
    }

    const char magic[] = "\x93NUMPY";
    file.write(magic, 6);

    uint8_t ver_major = 1, ver_minor = 0;
    file.write(reinterpret_cast<const char*>(&ver_major), 1);
    file.write(reinterpret_cast<const char*>(&ver_minor), 1);
    file.write(reinterpret_cast<const char*>(&header_len), 2);
    file.write(header_dict.data(), static_cast<std::streamsize>(header_dict.size()));
    file.write(reinterpret_cast<const char*>(arr.data.data()),
               static_cast<std::streamsize>(arr.data.size()));
}

}  // namespace iris::npy
