#pragma once

#include <any>
#include <functional>
#include <string>
#include <unordered_map>

#include <iris/core/error.hpp>

namespace iris {

/// Signature for a callback dispatch function that bridges a type-erased
/// validator to a strongly-typed run() call on the node's output.
using CallbackDispatchFn = std::function<Result<void>(
    const std::any& validator,
    const std::any& node_output)>;

/// Returns the global callback dispatch table mapping validator class_name
/// to CallbackDispatchFn.
const std::unordered_map<std::string, CallbackDispatchFn>& callback_dispatch_table();

/// Look up a callback dispatch function by class_name.
Result<CallbackDispatchFn> lookup_callback_dispatch(const std::string& class_name);

}  // namespace iris
