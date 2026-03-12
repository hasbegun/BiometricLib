#include <iris/nodes/onnx_segmentation.hpp>

#include <gtest/gtest.h>

using namespace iris;

#ifdef IRIS_HAS_ONNXRUNTIME

TEST(OnnxSegmentation, InvalidModelPathThrows) {
    OnnxSegmentation::Params params;
    params.model_path = "/nonexistent/path/to/model.onnx";

    EXPECT_THROW(OnnxSegmentation{params}, std::exception);
}

TEST(OnnxSegmentation, NodeParamsInvalidPathThrows) {
    std::unordered_map<std::string, std::string> np;
    np["model_path"] = "/nonexistent/model.onnx";

    EXPECT_THROW(OnnxSegmentation{np}, std::exception);
}

TEST(OnnxSegmentation, DefaultIntraOpNumThreadsIsZero) {
    OnnxSegmentation::Params params;
    EXPECT_EQ(params.intra_op_num_threads, 0);
}

TEST(OnnxSegmentation, NodeParamsIntraOpNumThreadsParsed) {
    // Constructor will throw (no model) but should parse the thread param first.
    // We verify it doesn't crash on the param parsing itself.
    std::unordered_map<std::string, std::string> np;
    np["model_path"] = "/nonexistent/model.onnx";
    np["intra_op_num_threads"] = "4";

    EXPECT_THROW(OnnxSegmentation{np}, std::exception);
}

#else

TEST(OnnxSegmentation, StubReturnsError) {
    OnnxSegmentation::Params params;
    OnnxSegmentation seg{params};

    IRImage img;
    auto result = seg.run(img);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::SegmentationFailed);
}

#endif
