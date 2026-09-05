# Performance Benchmarks — bench/CMakeLists.txt
# Benchmarks for per-script and per-component performance.

add_executable(luna_bench
    bench_stub.cpp
    bench_pipeline.cpp
)

target_link_libraries(luna_bench
    PRIVATE
        luna_lib
        benchmark::benchmark_main
)

target_compile_options(luna_bench
    PRIVATE
        ${LUNA_WARN_FLAGS}
        # Benchmarks always built without sanitizers (they distort timing).
)

target_compile_features(luna_bench PRIVATE cxx_std_20)

# Register benchmark as a CTest test (just runs the binary; results to stdout).
add_test(
    NAME luna_bench_smoke
    COMMAND luna_bench --benchmark_min_time=0.1s
)
