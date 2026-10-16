# ============================================================================
# MicroArchSim Tests
# ============================================================================

# Helper function to add a test executable
function(add_sim_test TEST_NAME)
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME} PRIVATE microarchsim_lib GTest::gtest_main)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Phase 1 tests
add_sim_test(test_instruction)
add_sim_test(test_register_file)

# Phase 2 tests
add_sim_test(test_reference_interpreter)

# Phase 3-4 tests
add_sim_test(test_pipeline)
add_sim_test(test_hazards)

# Phase 5 tests
add_sim_test(test_cache)

# Phase 6 tests
add_sim_test(test_memory_integration)

# Phase 8 tests
add_sim_test(test_differential)
