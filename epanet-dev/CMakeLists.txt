# CMakeLists.txt

cmake_minimum_required(VERSION 3.10)  # Updated to a more recent version for better features

# Project Name
project(EPANET3)

# Find MPI
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

# Set Output Directories
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# Enable Folders in IDEs (e.g., Visual Studio)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Compiler Flags
set(CMAKE_C_FLAGS "-std=c99")  # For C sources

if(MSVC)
    # For Microsoft Visual Studio
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /W4 /wd4100 /wd4571")
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
else()
    # For GCC/Clang
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    
    # set(CMAKE_CXX_FLAGS_DEBUG "-g -O0") # Debug flags: no optimization, include symbols
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pg -g")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg -g")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -pg -g")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -pg -g")
endif()

# Add below the existing debug flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer")
    option(USE_SANITIZER "Enable sanitizer instrumentation" OFF)
    if(USE_SANITIZER)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fsanitize=undefined")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address -fsanitize=undefined")
    endif()
endif()

# -------------------------
# EPANET3 Library Setup
# -------------------------

# EPANET3 Library Sources
set(epanet_lib_sources
    src/Core/datamanager.cpp
    src/Core/diagnostics.cpp
    src/Core/epanet3.cpp
    src/Core/error.cpp
    src/Core/hydbalance.cpp
    src/Core/hydengine.cpp
    src/Core/network.cpp
    src/Core/options.cpp
    src/Core/project.cpp
    src/Core/qualbalance.cpp
    src/Core/qualengine.cpp
    src/Core/units.cpp
    src/Elements/control.cpp
    src/Elements/curve.cpp
    src/Elements/demand.cpp
    src/Elements/element.cpp
    src/Elements/emitter.cpp
    src/Elements/junction.cpp
    src/Elements/link.cpp
    src/Elements/node.cpp
    src/Elements/pattern.cpp
    src/Elements/pipe.cpp
    src/Elements/pump.cpp
    src/Elements/pumpcurve.cpp
    src/Elements/qualsource.cpp
    src/Elements/reservoir.cpp
    src/Elements/tank.cpp
    src/Elements/valve.cpp
    src/Input/controlparser.cpp
    src/Input/curveparser.cpp
    src/Input/inputparser.cpp
    src/Input/inputreader.cpp
    src/Input/linkparser.cpp
    src/Input/nodeparser.cpp
    src/Input/optionparser.cpp
    src/Input/patternparser.cpp
    src/Models/demandmodel.cpp
    src/Models/headlossmodel.cpp
    src/Models/leakagemodel.cpp
    src/Models/pumpenergy.cpp
    src/Models/qualmodel.cpp
    src/Models/tankmixmodel.cpp
    src/Output/outputfile.cpp
    src/Output/projectwriter.cpp
    src/Output/reportfields.cpp
    src/Output/reportwriter.cpp
    src/Solvers/ggasolver.cpp
    src/Solvers/hydsolver.cpp
    src/Solvers/ltdsolver.cpp
    src/Solvers/matrixsolver.cpp
    src/Solvers/qualsolver.cpp
    src/Solvers/sparspak.cpp
    src/Solvers/sparspaksolver.cpp
    src/Utilities/graph.cpp
    src/Utilities/mempool.cpp
    src/Utilities/segpool.cpp
    src/Utilities/utilities.cpp
)

# EPANET3 Library Headers
set(epanet_lib_headers
    src/Core/constants.h
    src/Core/datamanager.h
    src/Core/diagnostics.h
    src/Core/error.h
    src/Core/hydbalance.h
    src/Core/hydengine.h
    src/Core/network.h
    src/Core/options.h
    src/Core/project.h
    src/Core/qualbalance.h
    src/Core/qualengine.h
    src/Core/units.h
    src/Elements/control.h
    src/Elements/curve.h
    src/Elements/demand.h
    src/Elements/element.h
    src/Elements/emitter.h
    src/Elements/junction.h
    src/Elements/link.h
    src/Elements/node.h
    src/Elements/pattern.h
    src/Elements/pipe.h
    src/Elements/pump.h
    src/Elements/pumpcurve.h
    src/Elements/qualsource.h
    src/Elements/reservoir.h
    src/Elements/tank.h
    src/Elements/valve.h
    src/Input/controlparser.h
    src/Input/curveparser.h
    src/Input/inputparser.h
    src/Input/inputreader.h
    src/Input/linkparser.h
    src/Input/nodeparser.h
    src/Input/optionparser.h
    src/Input/patternparser.h
    src/Models/demandmodel.h
    src/Models/headlossmodel.h
    src/Models/leakagemodel.h
    src/Models/pumpenergy.h
    src/Models/qualmodel.h
    src/Models/tankmixmodel.h
    src/Output/outputfile.h
    src/Output/projectwriter.h
    src/Output/reportfields.h
    src/Output/reportwriter.h
    src/Solvers/ggasolver.h
    src/Solvers/hydsolver.h
    src/Solvers/ltdsolver.h
    src/Solvers/matrixsolver.h
    src/Solvers/qualsolver.h
    src/Solvers/sparspak.h
    src/Solvers/sparspaksolver.h
    src/Utilities/graph.h
    src/Utilities/mempool.h
    src/Utilities/segpool.h
    src/Utilities/utilities.h
)

# Organize Source Groups for IDEs
source_group("Core" REGULAR_EXPRESSION "src/Core/.*")
source_group("Elements" REGULAR_EXPRESSION "src/Elements/.*")
source_group("Input" REGULAR_EXPRESSION "src/Input/.*")
source_group("Models" REGULAR_EXPRESSION "src/Models/.*")
source_group("Output" REGULAR_EXPRESSION "src/Output/.*")
source_group("Solvers" REGULAR_EXPRESSION "src/Solvers/.*")
source_group("Utilities" REGULAR_EXPRESSION "src/Utilities/.*")

# Include Directories for EPANET3 Library
include_directories(
    src
)

# Create EPANET3 Shared Library
add_library(epanet3 SHARED ${epanet_lib_sources} ${epanet_lib_headers})

# -------------------------
# run-epanet3 Executable Setup
# -------------------------

# Define Executable Source Files
set(run_epanet3_sources
    src/CLI/main.cpp
    src/CLI/Utils.cpp
    src/CLI/BBCounter.cpp
    src/CLI/BBStats.cpp
    src/CLI/ColorStream.cpp
    src/CLI/Helper.cpp
    src/CLI/BBSolver.cpp
)

# Include Directories for CLI (if separate headers are used)
include_directories(
    src/CLI
)

# Add Executable
add_executable(run-epanet3 ${run_epanet3_sources})

# Ensure Compatibility with <filesystem>
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
    target_link_libraries(epanet3 PRIVATE stdc++fs)
    target_link_libraries(run-epanet3 PRIVATE stdc++fs)
endif()

# Link the EPANET3 Library to the Executable
target_link_libraries(run-epanet3 PUBLIC epanet3)
target_link_libraries(run-epanet3 PRIVATE MPI::MPI_CXX)

# -------------------------
# Optional: Link Additional Libraries
# -------------------------
# If your project depends on other libraries (e.g., pthread), link them here
# Example:
# target_link_libraries(run-epanet3 PUBLIC epanet3 pthread)

# -------------------------
# Optional: Install Targets
# -------------------------
# If you plan to install the library and executable, define installation rules
# install(TARGETS epanet3
#     LIBRARY DESTINATION lib
#     ARCHIVE DESTINATION lib
#     RUNTIME DESTINATION bin
# )
# install(FILES ${epanet_lib_headers} DESTINATION include)
