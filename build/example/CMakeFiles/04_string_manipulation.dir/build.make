# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /workspaces/strutil

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /workspaces/strutil/build

# Include any dependencies generated for this target.
include example/CMakeFiles/04_string_manipulation.dir/depend.make

# Include the progress variables for this target.
include example/CMakeFiles/04_string_manipulation.dir/progress.make

# Include the compile flags for this target's objects.
include example/CMakeFiles/04_string_manipulation.dir/flags.make

example/CMakeFiles/04_string_manipulation.dir/04_string_manipulation.c.o: example/CMakeFiles/04_string_manipulation.dir/flags.make
example/CMakeFiles/04_string_manipulation.dir/04_string_manipulation.c.o: ../example/04_string_manipulation.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/workspaces/strutil/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object example/CMakeFiles/04_string_manipulation.dir/04_string_manipulation.c.o"
	cd /workspaces/strutil/build/example && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/04_string_manipulation.dir/04_string_manipulation.c.o   -c /workspaces/strutil/example/04_string_manipulation.c

example/CMakeFiles/04_string_manipulation.dir/04_string_manipulation.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/04_string_manipulation.dir/04_string_manipulation.c.i"
	cd /workspaces/strutil/build/example && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /workspaces/strutil/example/04_string_manipulation.c > CMakeFiles/04_string_manipulation.dir/04_string_manipulation.c.i

example/CMakeFiles/04_string_manipulation.dir/04_string_manipulation.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/04_string_manipulation.dir/04_string_manipulation.c.s"
	cd /workspaces/strutil/build/example && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /workspaces/strutil/example/04_string_manipulation.c -o CMakeFiles/04_string_manipulation.dir/04_string_manipulation.c.s

# Object files for target 04_string_manipulation
04_string_manipulation_OBJECTS = \
"CMakeFiles/04_string_manipulation.dir/04_string_manipulation.c.o"

# External object files for target 04_string_manipulation
04_string_manipulation_EXTERNAL_OBJECTS =

example/04_string_manipulation: example/CMakeFiles/04_string_manipulation.dir/04_string_manipulation.c.o
example/04_string_manipulation: example/CMakeFiles/04_string_manipulation.dir/build.make
example/04_string_manipulation: libstrutil.a
example/04_string_manipulation: example/CMakeFiles/04_string_manipulation.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/workspaces/strutil/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable 04_string_manipulation"
	cd /workspaces/strutil/build/example && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/04_string_manipulation.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
example/CMakeFiles/04_string_manipulation.dir/build: example/04_string_manipulation

.PHONY : example/CMakeFiles/04_string_manipulation.dir/build

example/CMakeFiles/04_string_manipulation.dir/clean:
	cd /workspaces/strutil/build/example && $(CMAKE_COMMAND) -P CMakeFiles/04_string_manipulation.dir/cmake_clean.cmake
.PHONY : example/CMakeFiles/04_string_manipulation.dir/clean

example/CMakeFiles/04_string_manipulation.dir/depend:
	cd /workspaces/strutil/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /workspaces/strutil /workspaces/strutil/example /workspaces/strutil/build /workspaces/strutil/build/example /workspaces/strutil/build/example/CMakeFiles/04_string_manipulation.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : example/CMakeFiles/04_string_manipulation.dir/depend

