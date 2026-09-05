#pragma once
// Sandboxed Execution Service Wrapper for the Gravity VM.
// Provides an isolated API for executing untrusted Luna scripts with strict resource budgets.
#include "../vm/VM.hpp"
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace luna::service {

enum class ServiceStatus {
    Success,
    CompileError,
    QuotaExceeded,
    RuntimeError
};

struct SandboxRequest {
    std::string code;
    size_t      max_instructions{10'000'000};
    size_t      max_frames{512};
    size_t      max_stack{16384};
};

struct SandboxResponse {
    ServiceStatus status{ServiceStatus::Success};
    std::string   output;
    std::string   error_message;
    size_t        instructions_executed{0};
    int64_t       duration_us{0};
};

class SandboxService {
public:
    SandboxService() = default;

    [[nodiscard]] SandboxResponse execute(const SandboxRequest& request);
    [[nodiscard]] std::string execute_json(std::string_view json_request);
};

} // namespace luna::service
