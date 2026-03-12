#include <iris/nodes/regular_probe_schema.hpp>

#include <cmath>

#include <gtest/gtest.h>

using namespace iris;

TEST(RegularProbeSchema, DefaultGrid) {
    RegularProbeSchema schema;
    EXPECT_EQ(schema.rhos().size(), 16u * 256u);
    EXPECT_EQ(schema.phis().size(), 16u * 256u);
}

TEST(RegularProbeSchema, RhosRange) {
    RegularProbeSchema schema;
    for (double rho : schema.rhos()) {
        EXPECT_GE(rho, 0.0);
        EXPECT_LE(rho, 1.0);
    }
    // First rho should be boundary_rho_low = 0.0
    EXPECT_NEAR(schema.rhos()[0], 0.0, 1e-10);
    // Last row's rho should be 1.0 - boundary_rho_high = 0.9375
    EXPECT_NEAR(schema.rhos().back(), 1.0 - 0.0625, 1e-10);
}

TEST(RegularProbeSchema, PhisRange) {
    RegularProbeSchema schema;
    for (double phi : schema.phis()) {
        EXPECT_GE(phi, 0.0);
        EXPECT_LT(phi, 1.0);
    }
    // First phi should be 0.0 (periodic-left default)
    EXPECT_NEAR(schema.phis()[0], 0.0, 1e-10);
}

TEST(RegularProbeSchema, PeriodicSymmetric) {
    RegularProbeSchema::Params params;
    params.boundary_phi = "periodic-symmetric";
    RegularProbeSchema schema{params};

    // With periodic-symmetric, first phi should be half the spacing
    const double spacing = 1.0 / 256.0;
    EXPECT_NEAR(schema.phis()[0], spacing / 2.0, 1e-10);
}

TEST(RegularProbeSchema, CustomSize) {
    RegularProbeSchema::Params params;
    params.n_rows = 8;
    params.n_cols = 64;
    RegularProbeSchema schema{params};

    EXPECT_EQ(schema.rhos().size(), 8u * 64u);
    EXPECT_EQ(schema.phis().size(), 8u * 64u);
}

TEST(RegularProbeSchema, NodeParamsConstruction) {
    std::unordered_map<std::string, std::string> np;
    np["n_rows"] = "8";
    np["n_cols"] = "128";
    np["boundary_rho_low"] = "0.1";
    np["boundary_rho_high"] = "0.1";
    np["boundary_phi"] = "periodic-symmetric";
    RegularProbeSchema schema{np};

    EXPECT_EQ(schema.rhos().size(), 8u * 128u);
    EXPECT_NEAR(schema.rhos()[0], 0.1, 1e-10);
}
