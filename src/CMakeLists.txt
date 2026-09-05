# Compiler toolchain & Gravity VM — src/CMakeLists.txt
# Builds the core library and the compiler executable.

# Core static library
add_library(luna_lib STATIC
    stub.cpp
    lexer/Lexer.cpp
    gravity/Gravity.cpp
    parser/Parser.cpp
    resolver/Resolver.cpp
    compiler/Compiler.cpp
    compiler/Disassembler.cpp
    interpreter/TreeWalker.cpp
    service/SandboxService.cpp
    vm/Value.cpp
    vm/VM.cpp
)

target_include_directories(luna_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(luna_lib
    PRIVATE
        ${LUNA_WARN_FLAGS}
        ${LUNA_OPT_FLAGS}
        ${LUNA_SANITIZE_FLAGS}
)

target_link_options(luna_lib
    PRIVATE
        ${LUNA_SANITIZE_FLAGS}
)

target_compile_features(luna_lib PUBLIC cxx_std_26)

# Luna executable
add_executable(luna main.cpp)

target_link_libraries(luna PRIVATE luna_lib)

target_compile_options(luna
    PRIVATE
        ${LUNA_WARN_FLAGS}
        ${LUNA_OPT_FLAGS}
        ${LUNA_SANITIZE_FLAGS}
)

target_link_options(luna
    PRIVATE
        ${LUNA_SANITIZE_FLAGS}
)

target_compile_features(luna PRIVATE cxx_std_26)
