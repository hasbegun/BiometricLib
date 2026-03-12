#pragma once

#include <string>
#include <string_view>

namespace iris {

/// CRTP base for pipeline algorithm nodes.
///
/// Derived classes implement:
///   auto run(InputType input) -> Result<OutputType>;
///
/// The CRTP pattern gives us static polymorphism with zero vtable overhead
/// in the hot path. The pipeline executor uses type-erased wrappers
/// (PipelineNodeBase) for dynamic dispatch at the DAG level only.
template <typename Derived>
class Algorithm {
public:
    /// Access the concrete derived class.
    Derived& self() noexcept { return static_cast<Derived&>(*this); }
    const Derived& self() const noexcept { return static_cast<const Derived&>(*this); }

    /// Name of this algorithm (for logging/debugging).
    [[nodiscard]] std::string_view name() const noexcept { return self().kName; }

protected:
    Algorithm() = default;
    ~Algorithm() = default;
    Algorithm(const Algorithm&) = default;
    Algorithm& operator=(const Algorithm&) = default;
    Algorithm(Algorithm&&) = default;
    Algorithm& operator=(Algorithm&&) = default;
};

}  // namespace iris
