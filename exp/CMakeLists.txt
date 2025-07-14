# CMakeLists.txt for exp directory
# Compiles token_dist.cpp with gflags dependency

# Find gflags package
find_package(gflags REQUIRED)

# Create executable for token_dist
add_executable(token_dist token_dist.cpp)

# Link with gflags
target_link_libraries(token_dist gflags)

# Set C++ standard
set_target_properties(token_dist PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Add compile definitions if needed
target_compile_definitions(token_dist PRIVATE
    $<$<BOOL:${BUILD_UNIT_TESTS}>:BUILD_UNIT_TESTS>
) 