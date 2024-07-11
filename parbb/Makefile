# Define the compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -fopenmp
RELEASE_FLAGS = -O2
DEBUG_FLAGS = -g -O0

# Define the target executables
TARGET_RELEASE = parbb.bin
TARGET_DEBUG = parbb_debug.bin

# Define the source files
SRCS = parbb.cpp

# Define the object files
OBJS_RELEASE = $(SRCS:.cpp=.o)
OBJS_DEBUG = $(SRCS:.cpp=_debug.o)

# Default target
all: release debug clean_objs

# Release build
release: $(TARGET_RELEASE)

# Debug build
debug: $(TARGET_DEBUG)

# Link the object files to create the release executable
$(TARGET_RELEASE): $(OBJS_RELEASE)
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) -o $(TARGET_RELEASE) $(OBJS_RELEASE)

# Link the object files to create the debug executable
$(TARGET_DEBUG): $(OBJS_DEBUG)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) -o $(TARGET_DEBUG) $(OBJS_DEBUG)

# Compile the source files into object files for release
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) -c $< -o $@

# Compile the source files into object files for debug
%_debug.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) -c $< -o $@

# Clean up object files and the executables
clean:
	rm -f $(OBJS_RELEASE) $(OBJS_DEBUG) $(TARGET_RELEASE) $(TARGET_DEBUG)

# Clean up object files only
clean_objs:
	rm -f $(OBJS_RELEASE) $(OBJS_DEBUG)

.PHONY: all release debug clean clean_objs
