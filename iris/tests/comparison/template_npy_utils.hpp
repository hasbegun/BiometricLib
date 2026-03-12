#pragma once

#include <filesystem>

#include <iris/core/error.hpp>
#include <iris/core/iris_code_packed.hpp>

#include "npy_reader.hpp"
#include "npy_writer.hpp"

namespace iris::npy {

/// Write a PackedIrisCode to two .npy files (code + mask), each 16×512 uint8.
inline void write_packed_code(const std::filesystem::path& code_path,
                               const std::filesystem::path& mask_path,
                               const PackedIrisCode& code) {
    auto [code_mat, mask_mat] = code.to_mat();
    write_npy(code_path, code_mat);
    write_npy(mask_path, mask_mat);
}

/// Read a PackedIrisCode from two .npy files (code + mask).
inline Result<PackedIrisCode> read_packed_code(
    const std::filesystem::path& code_path,
    const std::filesystem::path& mask_path) {
    try {
        auto code_arr = read_npy(code_path);
        auto mask_arr = read_npy(mask_path);

        auto code_mat = code_arr.to_mat();
        auto mask_mat = mask_arr.to_mat();

        return PackedIrisCode::from_mat(code_mat, mask_mat);
    } catch (const std::exception& e) {
        return make_error(ErrorCode::IoFailed,
                          "Failed to read packed code from NPY", e.what());
    }
}

}  // namespace iris::npy
