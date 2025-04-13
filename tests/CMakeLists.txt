# Add the test executable
add_executable(test_strutil test.c)

# Link the library and CUnit to the test executable
target_link_libraries(test_strutil PRIVATE strutil pthread cunit)

# Add a custom target to run the tests
add_custom_target(run_tests COMMAND test_strutil DEPENDS test_strutil)