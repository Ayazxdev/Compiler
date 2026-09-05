// Implementation of Sandboxed Execution Service for Gravity VM.
#include "SandboxService.hpp"
#include <chrono>
#include <format>
#include <sstream>

namespace luna::service {

SandboxResponse SandboxService::execute(const SandboxRequest& request) {
    SandboxResponse response;
    auto start_time = std::chrono::high_resolution_clock::now();

    vm::ResourceLimits limits;
    limits.max_instructions = request.max_instructions;
    limits.max_frames       = request.max_frames;
    limits.max_stack        = request.max_stack;

    vm::VM vm(limits);
    std::string stdout_capture;
    vm.set_output_handler([&stdout_capture](std::string_view text) {
        stdout_capture.append(text);
    });

    auto res = vm.interpret(request.code);
    auto end_time = std::chrono::high_resolution_clock::now();

    response.output = std::move(stdout_capture);
    response.instructions_executed = vm.instructions_executed();
    response.duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    switch (res) {
        case vm::InterpretResult::Ok:
            response.status = ServiceStatus::Success;
            break;
        case vm::InterpretResult::CompileError:
            response.status = ServiceStatus::CompileError;
            response.error_message = "Syntax or Compilation Error.";
            break;
        case vm::InterpretResult::RuntimeError:
            if (response.instructions_executed >= limits.max_instructions) {
                response.status = ServiceStatus::QuotaExceeded;
                response.error_message = "Instruction step quota exceeded.";
            } else {
                response.status = ServiceStatus::RuntimeError;
                response.error_message = "Runtime execution failure.";
            }
            break;
    }

    return response;
}

std::string SandboxService::execute_json(std::string_view) {
    // Basic JSON envelope response
    SandboxRequest req;
    auto resp = execute(req);
    return std::format(
        "{{\"status\":\"{}\",\"instructions\":{},\"duration_us\":{}}}",
        resp.status == ServiceStatus::Success ? "ok" : "error",
        resp.instructions_executed,
        resp.duration_us
    );
}

} // namespace luna::service
