#include <eyetrack/core/error.hpp>

namespace eyetrack {

std::ostream& operator<<(std::ostream& os, const EyetrackError& err) {
    os << "EyetrackError{" << error_code_name(err.code);
    if (!err.message.empty()) {
        os << ": " << err.message;
    }
    if (!err.detail.empty()) {
        os << " (" << err.detail << ")";
    }
    os << "}";
    return os;
}

}  // namespace eyetrack
