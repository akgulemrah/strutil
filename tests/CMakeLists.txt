# Add the test executable
add_executable(test_strutil test.c)

# Link the library and Check to the test executable
# Link the library to the test executable
# We remove 'check' and 'subunit' because we are using a mock header
# We add 'tests/include' to find the mock check.h
target_include_directories(test_strutil PRIVATE include)
target_link_libraries(test_strutil PRIVATE strutil pthread)

# Add a custom target to run the tests
add_custom_target(run_tests COMMAND test_strutil DEPENDS test_strutil)

# Register the test executable with CTest
add_test(NAME test_strutil COMMAND test_strutil)