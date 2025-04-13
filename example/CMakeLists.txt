# Add example executables
add_executable(01_basic_operations 01_basic_operations.c)
add_executable(02_thread_safe 02_thread_safe.c)
add_executable(03_io_operations 03_io_operations.c)
add_executable(04_string_manipulation 04_string_manipulation.c)

# Link the library to the examples
target_link_libraries(01_basic_operations PRIVATE strutil pthread)
target_link_libraries(02_thread_safe PRIVATE strutil pthread)
target_link_libraries(03_io_operations PRIVATE strutil pthread)
target_link_libraries(04_string_manipulation PRIVATE strutil pthread)