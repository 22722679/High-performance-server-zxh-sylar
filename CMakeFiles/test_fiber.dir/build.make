# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Produce verbose output by default.
VERBOSE = 1

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

# The program to use to edit the cache.
CMAKE_EDIT_COMMAND = /usr/bin/ccmake

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /root/sylar-project/sylar

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /root/sylar-project/sylar

# Include any dependencies generated for this target.
include CMakeFiles/test_fiber.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/test_fiber.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/test_fiber.dir/flags.make

CMakeFiles/test_fiber.dir/tests/test_fiber.cc.o: CMakeFiles/test_fiber.dir/flags.make
CMakeFiles/test_fiber.dir/tests/test_fiber.cc.o: tests/test_fiber.cc
	$(CMAKE_COMMAND) -E cmake_progress_report /root/sylar-project/sylar/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object CMakeFiles/test_fiber.dir/tests/test_fiber.cc.o"
	/usr/bin/c++   $(CXX_DEFINES) -D__FILE__=\"tests/test_fiber.cc\" $(CXX_FLAGS) -o CMakeFiles/test_fiber.dir/tests/test_fiber.cc.o -c /root/sylar-project/sylar/tests/test_fiber.cc

CMakeFiles/test_fiber.dir/tests/test_fiber.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test_fiber.dir/tests/test_fiber.cc.i"
	/usr/bin/c++  $(CXX_DEFINES) -D__FILE__=\"tests/test_fiber.cc\" $(CXX_FLAGS) -E /root/sylar-project/sylar/tests/test_fiber.cc > CMakeFiles/test_fiber.dir/tests/test_fiber.cc.i

CMakeFiles/test_fiber.dir/tests/test_fiber.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test_fiber.dir/tests/test_fiber.cc.s"
	/usr/bin/c++  $(CXX_DEFINES) -D__FILE__=\"tests/test_fiber.cc\" $(CXX_FLAGS) -S /root/sylar-project/sylar/tests/test_fiber.cc -o CMakeFiles/test_fiber.dir/tests/test_fiber.cc.s

CMakeFiles/test_fiber.dir/tests/test_fiber.cc.o.requires:
.PHONY : CMakeFiles/test_fiber.dir/tests/test_fiber.cc.o.requires

CMakeFiles/test_fiber.dir/tests/test_fiber.cc.o.provides: CMakeFiles/test_fiber.dir/tests/test_fiber.cc.o.requires
	$(MAKE) -f CMakeFiles/test_fiber.dir/build.make CMakeFiles/test_fiber.dir/tests/test_fiber.cc.o.provides.build
.PHONY : CMakeFiles/test_fiber.dir/tests/test_fiber.cc.o.provides

CMakeFiles/test_fiber.dir/tests/test_fiber.cc.o.provides.build: CMakeFiles/test_fiber.dir/tests/test_fiber.cc.o

# Object files for target test_fiber
test_fiber_OBJECTS = \
"CMakeFiles/test_fiber.dir/tests/test_fiber.cc.o"

# External object files for target test_fiber
test_fiber_EXTERNAL_OBJECTS =

bin/test_fiber: CMakeFiles/test_fiber.dir/tests/test_fiber.cc.o
bin/test_fiber: CMakeFiles/test_fiber.dir/build.make
bin/test_fiber: lib/libsylar.so
bin/test_fiber: CMakeFiles/test_fiber.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable bin/test_fiber"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_fiber.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/test_fiber.dir/build: bin/test_fiber
.PHONY : CMakeFiles/test_fiber.dir/build

CMakeFiles/test_fiber.dir/requires: CMakeFiles/test_fiber.dir/tests/test_fiber.cc.o.requires
.PHONY : CMakeFiles/test_fiber.dir/requires

CMakeFiles/test_fiber.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/test_fiber.dir/cmake_clean.cmake
.PHONY : CMakeFiles/test_fiber.dir/clean

CMakeFiles/test_fiber.dir/depend:
	cd /root/sylar-project/sylar && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /root/sylar-project/sylar /root/sylar-project/sylar /root/sylar-project/sylar /root/sylar-project/sylar /root/sylar-project/sylar/CMakeFiles/test_fiber.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/test_fiber.dir/depend

