# Compiler
CXX = mpicxx

# Build type (default is debug)
BUILD_TYPE ?= debug

# Base compiler flags
CXXFLAGS_BASE = -std=c++17 -Isrc -MMD -MP -fPIC -I/usr/include/nlohmann -fopenmp # Add JSON library path here

# Compiler flags for different build types
ifeq ($(BUILD_TYPE), valgrind)
    CXXFLAGS = $(CXXFLAGS_BASE) -g -O0
    LDFLAGS = -Wl,-rpath,'$$ORIGIN' -fopenmp
else ifeq ($(BUILD_TYPE), debug)
    CXXFLAGS = $(CXXFLAGS_BASE) -g -pg
    LDFLAGS = -pg -Wl,-rpath,'$$ORIGIN' -fopenmp
else ifeq ($(BUILD_TYPE), release)
    CXXFLAGS = $(CXXFLAGS_BASE) -O3 -DNDEBUG -march=native -funroll-loops -fomit-frame-pointer -flto
    LDFLAGS = -flto -Wl,-rpath,'$$ORIGIN' -fopenmp
else
    $(error Invalid BUILD_TYPE specified. Use 'debug' or 'release'.)
endif

# Libraries to link
LIBS =

# Optional filesystem library (for GCC < 8)
FS_LIB =

# Source directories
SRC_DIR = src

# Find all .cpp files in src/ and its subdirectories
SRCS = $(shell find $(SRC_DIR) -name '*.cpp')

# Separate library and executable source files
SRCS_EXE = $(wildcard src/CLI/*.cpp)
SRCS_LIB = $(filter-out $(SRCS_EXE), $(SRCS))

# Object files for the library
OBJS_LIB = $(patsubst %.cpp,$(BUILD_TYPE)/%.o,$(SRCS_LIB))

# Object files for the executable
OBJS_EXE = $(patsubst %.cpp,$(BUILD_TYPE)/%.o,$(SRCS_EXE))

# Dependency files
DEPS_LIB = $(OBJS_LIB:.o=.d)
DEPS_EXE = $(OBJS_EXE:.o=.d)

# Targets
TARGET_LIB = $(BUILD_TYPE)/libepanet3.so
TARGET_EXE = $(BUILD_TYPE)/run-epanet3

# Default target
all: $(TARGET_EXE)

# Rule to build the shared library
$(TARGET_LIB): $(OBJS_LIB)
	@mkdir -p $(BUILD_TYPE)
	$(CXX) -shared -o $@ $(OBJS_LIB) $(LDFLAGS) $(LIBS) $(FS_LIB)

# Rule to build the executable
$(TARGET_EXE): $(OBJS_EXE) $(TARGET_LIB)
	@mkdir -p $(BUILD_TYPE)
	$(CXX) -o $@ $(OBJS_EXE) -L$(BUILD_TYPE) -lepanet3 $(LDFLAGS) $(LIBS) $(FS_LIB)

run_debug: BUILD_TYPE = debug
run_debug: $(TARGET_EXE)
	./$(TARGET_EXE)

run_release: BUILD_TYPE = release
run_release: $(TARGET_EXE)
	./$(TARGET_EXE) --h_max 12 --max_actuations 1 --interval_sync 2048 

# Pattern rule to compile .cpp to .o and generate dependencies
$(BUILD_TYPE)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -rf debug/* 
	rm -rf release/*

# Include dependency files
-include $(DEPS_LIB)
-include $(DEPS_EXE)

# Generate call tree using Valgrind's Callgrind tool and visualize it
call_tree: BUILD_TYPE = valgrind
call_tree: $(TARGET_EXE)
	@mkdir -p valgrind
	cd valgrind
	valgrind --tool=callgrind ../$(TARGET_EXE) --test test_cost_1
	gprof2dot -f callgrind callgrind.out.* > call_tree.dot

.PHONY: all clean run_debug run_release call_tree
