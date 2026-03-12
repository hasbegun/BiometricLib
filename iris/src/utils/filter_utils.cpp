#include <iris/utils/filter_utils.hpp>

#include <cmath>
#include <numbers>

#include <opencv2/core.hpp>

namespace iris::filter_utils {

std::pair<cv::Mat, cv::Mat> make_meshgrid(int phi_size, int rho_size) {
    const int phi_half = phi_size / 2;
    const int rho_half = rho_size / 2;

    // x varies along rows (rho), y varies along columns (phi)
    // Matches Python: y, x = meshgrid(arange(-phi_half, phi_half+1),
    //                                  arange(-rho_half, rho_half+1), indexing="xy")
    cv::Mat x(rho_size, phi_size, CV_64FC1);
    cv::Mat y(rho_size, phi_size, CV_64FC1);

    for (int r = 0; r < rho_size; ++r) {
        auto* xr = x.ptr<double>(r);
        auto* yr = y.ptr<double>(r);
        const double rv = static_cast<double>(r - rho_half);
        for (int c = 0; c < phi_size; ++c) {
            xr[c] = rv;
            yr[c] = static_cast<double>(c - phi_half);
        }
    }

    return {x, y};
}

std::pair<cv::Mat, cv::Mat> rotate_coords(const cv::Mat& x, const cv::Mat& y,
                                           double angle_degrees) {
    const double rad = angle_degrees * std::numbers::pi / 180.0;
    const double ct = std::cos(rad);
    const double st = std::sin(rad);

    cv::Mat rotx(x.rows, x.cols, CV_64FC1);
    cv::Mat roty(x.rows, x.cols, CV_64FC1);

    for (int r = 0; r < x.rows; ++r) {
        const auto* xr = x.ptr<double>(r);
        const auto* yr = y.ptr<double>(r);
        auto* rxr = rotx.ptr<double>(r);
        auto* ryr = roty.ptr<double>(r);
        for (int c = 0; c < x.cols; ++c) {
            rxr[c] = xr[c] * ct + yr[c] * st;
            ryr[c] = -xr[c] * st + yr[c] * ct;
        }
    }

    return {rotx, roty};
}

cv::Mat compute_radius(const cv::Mat& x, const cv::Mat& y) {
    cv::Mat radius(x.rows, x.cols, CV_64FC1);
    for (int r = 0; r < x.rows; ++r) {
        const auto* xr = x.ptr<double>(r);
        const auto* yr = y.ptr<double>(r);
        auto* rr = radius.ptr<double>(r);
        for (int c = 0; c < x.cols; ++c) {
            rr[c] = std::sqrt(xr[c] * xr[c] + yr[c] * yr[c]);
        }
    }
    return radius;
}

cv::Mat fftshift(const cv::Mat& src) {
    cv::Mat dst = src.clone();

    const int cx = src.cols / 2;
    const int cy = src.rows / 2;

    // Define quadrants
    cv::Mat q0(dst, cv::Rect(0, 0, cx, cy));      // top-left
    cv::Mat q1(dst, cv::Rect(cx, 0, src.cols - cx, cy));  // top-right
    cv::Mat q2(dst, cv::Rect(0, cy, cx, src.rows - cy));  // bottom-left
    cv::Mat q3(dst, cv::Rect(cx, cy, src.cols - cx, src.rows - cy));  // bottom-right

    // Swap diagonally: q0 ↔ q3, q1 ↔ q2
    cv::Mat tmp;
    q0.copyTo(tmp);
    q3.copyTo(q0);
    tmp.copyTo(q3);

    q1.copyTo(tmp);
    q2.copyTo(q1);
    tmp.copyTo(q2);

    return dst;
}

void normalize_frobenius(cv::Mat& real, cv::Mat& imag) {
    const double norm_real = cv::norm(real, cv::NORM_L2);
    if (norm_real > 0.0) {
        real /= norm_real;
    }

    const double norm_imag = cv::norm(imag, cv::NORM_L2);
    if (norm_imag > 0.0) {
        imag /= norm_imag;
    }
}

void apply_fixpoints(cv::Mat& real, cv::Mat& imag) {
    constexpr double scale = 32768.0;  // 2^15

    for (int r = 0; r < real.rows; ++r) {
        auto* rr = real.ptr<double>(r);
        auto* ir = imag.ptr<double>(r);
        for (int c = 0; c < real.cols; ++c) {
            rr[c] = std::round(rr[c] * scale);
            ir[c] = std::round(ir[c] * scale);
        }
    }
}

}  // namespace iris::filter_utils
