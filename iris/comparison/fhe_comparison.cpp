/// fhe_comparison — Compare plaintext and encrypted Hamming distance scores.
///
/// Usage:
///   fhe_comparison <templates_dir> <output_csv> [max_templates]
///
/// Loads iris code NPY files from templates_dir (from iris_comparison output),
/// computes plaintext HD and encrypted HD for each pair, writes results to CSV.
///
/// Output CSV columns:
///   image_a, image_b, plaintext_hd, encrypted_hd, abs_diff,
///   encrypt_ms, match_ms, decrypt_ms

#ifdef IRIS_HAS_FHE

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <iris/core/types.hpp>
#include <iris/crypto/encrypted_matcher.hpp>
#include <iris/crypto/encrypted_template.hpp>
#include <iris/crypto/fhe_context.hpp>

// NPY reader from tests/comparison/
#include "../tests/comparison/npy_reader.hpp"

// Suppress OpenFHE warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"

namespace fs = std::filesystem;
using namespace std::chrono;

namespace {

/// Load a PackedIrisCode from two NPY files (code + mask).
iris::PackedIrisCode load_template(const fs::path& code_path,
                                    const fs::path& mask_path) {
    auto code_npy = iris::npy::read_npy(code_path);
    auto mask_npy = iris::npy::read_npy(mask_path);
    auto code_mat = code_npy.to_mat();
    auto mask_mat = mask_npy.to_mat();

    return iris::PackedIrisCode::from_mat(code_mat, mask_mat);
}

/// Plaintext Hamming distance (same as in test_fhe_integration.cpp).
double plaintext_hd(const iris::PackedIrisCode& a,
                    const iris::PackedIrisCode& b) {
    size_t num = 0;
    size_t den = 0;
    for (size_t w = 0; w < iris::PackedIrisCode::kNumWords; ++w) {
        uint64_t mask = a.mask_bits[w] & b.mask_bits[w];
        uint64_t diff = (a.code_bits[w] ^ b.code_bits[w]) & mask;
        num += static_cast<size_t>(std::popcount(diff));
        den += static_cast<size_t>(std::popcount(mask));
    }
    if (den == 0) return 1.0;
    return static_cast<double>(num) / static_cast<double>(den);
}

/// Discover template names from NPY files in a directory.
std::vector<std::string> discover_templates(const fs::path& dir,
                                             size_t max_count) {
    std::vector<std::string> names;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() != ".npy") continue;
        auto stem = entry.path().stem().string();
        if (stem.ends_with("_code_0")) {
            auto name = stem.substr(0, stem.size() - 7);  // remove _code_0
            // Check mask file also exists
            if (fs::exists(dir / (name + "_mask_0.npy"))) {
                names.push_back(name);
            }
        }
    }
    std::sort(names.begin(), names.end());
    if (names.size() > max_count) {
        names.resize(max_count);
    }
    return names;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: fhe_comparison <templates_dir> <output_csv> "
                  << "[max_templates=20]\n";
        return 1;
    }

    fs::path templates_dir(argv[1]);
    fs::path output_csv(argv[2]);
    size_t max_templates = (argc >= 4) ? static_cast<size_t>(std::stoul(argv[3])) : 20;

    // Discover templates
    auto names = discover_templates(templates_dir, max_templates);
    std::cout << "Found " << names.size() << " templates in "
              << templates_dir << "\n";

    if (names.size() < 2) {
        std::cerr << "Need at least 2 templates for comparison\n";
        return 1;
    }

    // Load templates
    std::cout << "Loading templates...\n";
    std::vector<iris::PackedIrisCode> codes;
    codes.reserve(names.size());
    for (const auto& name : names) {
        codes.push_back(load_template(
            templates_dir / (name + "_code_0.npy"),
            templates_dir / (name + "_mask_0.npy")));
    }
    std::cout << "Loaded " << codes.size() << " templates\n";

    // Create FHE context and generate keys
    std::cout << "Creating FHE context and generating keys...\n";
    auto ctx_result = iris::FHEContext::create();
    if (!ctx_result) {
        std::cerr << "FHE context creation failed: "
                  << ctx_result.error().message << "\n";
        return 1;
    }
    auto& ctx = *ctx_result;
    auto key_result = ctx.generate_keys();
    if (!key_result) {
        std::cerr << "Key generation failed: "
                  << key_result.error().message << "\n";
        return 1;
    }
    std::cout << "FHE context ready (slots=" << ctx.slot_count() << ")\n";

    // Open output CSV
    fs::create_directories(output_csv.parent_path());
    std::ofstream csv(output_csv);
    csv << "image_a,image_b,plaintext_hd,encrypted_hd,abs_diff,"
        << "encrypt_ms,match_ms,decrypt_ms\n";

    size_t n = codes.size();
    size_t total_pairs = n * (n - 1) / 2;
    std::cout << "Comparing " << total_pairs << " pairs...\n";

    double max_diff = 0.0;
    double sum_diff = 0.0;
    size_t disagreements = 0;
    size_t pair_idx = 0;
    const double threshold = 0.35;

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            ++pair_idx;

            // Plaintext HD
            double pt_hd = plaintext_hd(codes[i], codes[j]);

            // Encrypt
            auto t0 = high_resolution_clock::now();
            auto enc_a = iris::EncryptedTemplate::encrypt(ctx, codes[i]);
            auto enc_b = iris::EncryptedTemplate::encrypt(ctx, codes[j]);
            auto t1 = high_resolution_clock::now();

            if (!enc_a || !enc_b) {
                std::cerr << "Encryption failed for pair " << names[i]
                          << " / " << names[j] << "\n";
                continue;
            }

            // Match encrypted
            auto t2 = high_resolution_clock::now();
            auto match_r = iris::EncryptedMatcher::match_encrypted(
                ctx, *enc_a, *enc_b);
            auto t3 = high_resolution_clock::now();

            if (!match_r) {
                std::cerr << "Encrypted match failed for pair " << names[i]
                          << " / " << names[j] << "\n";
                continue;
            }

            // Decrypt
            auto t4 = high_resolution_clock::now();
            auto dec_r = iris::EncryptedMatcher::decrypt_result(ctx, *match_r);
            auto t5 = high_resolution_clock::now();

            if (!dec_r) {
                std::cerr << "Decryption failed for pair " << names[i]
                          << " / " << names[j] << "\n";
                continue;
            }

            double enc_hd = *dec_r;
            double diff = std::abs(pt_hd - enc_hd);

            double encrypt_ms = static_cast<double>(
                duration_cast<microseconds>(t1 - t0).count()) / 1000.0;
            double match_ms = static_cast<double>(
                duration_cast<microseconds>(t3 - t2).count()) / 1000.0;
            double decrypt_ms = static_cast<double>(
                duration_cast<microseconds>(t5 - t4).count()) / 1000.0;

            csv << names[i] << "," << names[j] << ","
                << std::fixed << std::setprecision(12)
                << pt_hd << "," << enc_hd << "," << diff << ","
                << std::setprecision(2)
                << encrypt_ms << "," << match_ms << "," << decrypt_ms << "\n";

            max_diff = std::max(max_diff, diff);
            sum_diff += diff;

            bool pt_match = (pt_hd < threshold);
            bool enc_match = (enc_hd < threshold);
            if (pt_match != enc_match) ++disagreements;

            if (pair_idx % 10 == 0 || pair_idx == total_pairs) {
                std::cout << "  [" << pair_idx << "/" << total_pairs << "] "
                          << names[i] << " vs " << names[j]
                          << " — pt=" << std::setprecision(6) << pt_hd
                          << " enc=" << enc_hd
                          << " diff=" << std::scientific << diff
                          << std::fixed << "\n";
            }
        }
    }
    csv.close();

    // Summary
    double mean_diff = (pair_idx > 0) ? sum_diff / static_cast<double>(pair_idx) : 0.0;
    std::cout << "\n========================================\n"
              << "FHE Comparison Summary\n"
              << "========================================\n"
              << "Templates:         " << n << "\n"
              << "Pairs compared:    " << pair_idx << "\n"
              << "Max |pt - enc|:    " << std::scientific << max_diff << "\n"
              << "Mean |pt - enc|:   " << mean_diff << "\n"
              << "Disagreements@0.35:" << disagreements << "/" << pair_idx << "\n"
              << "Output:            " << output_csv << "\n"
              << "Verdict:           " << (max_diff < 1e-6 && disagreements == 0
                                             ? "PASS" : "FAIL") << "\n"
              << "========================================\n";

    return (max_diff < 1e-6 && disagreements == 0) ? 0 : 1;
}

#pragma GCC diagnostic pop

#else  // !IRIS_HAS_FHE

#include <iostream>

int main() {
    std::cerr << "fhe_comparison requires IRIS_ENABLE_FHE=ON\n";
    return 1;
}

#endif
