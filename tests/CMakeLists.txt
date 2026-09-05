# Test suite — tests/CMakeLists.txt
# GoogleTest test targets.

include(GoogleTest)

add_executable(luna_tests
    test_stub.cpp
    test_lexer.cpp
    test_pipeline.cpp
    test_differential.cpp
    test_golden.cpp
    test_sandbox.cpp
)

target_link_libraries(luna_tests
    PRIVATE
        luna_lib
        GTest::gtest_main
)

target_compile_options(luna_tests
    PRIVATE
        ${LUNA_WARN_FLAGS}
        ${LUNA_SANITIZE_FLAGS}
)

target_link_options(luna_tests
    PRIVATE
        ${LUNA_SANITIZE_FLAGS}
)

target_compile_features(luna_tests PRIVATE cxx_std_20)

# Register with CTest so `ctest` / `cmake --test` finds them automatically.
gtest_discover_tests(luna_tests)
