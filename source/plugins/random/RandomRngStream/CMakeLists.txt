cmake_minimum_required(VERSION 3.28)
set(ProjectName RandomRngStream)

project(${ProjectName})

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Header and Source files
set(HEADERS
    src/RandomRngStream.h
    src/RandomRngStreamFactory.h
)
set(SOURCES 
    src/RandomRngStream.cpp
    src/RandomRngStreamFactory.cpp
)

# Add the plugin as a shared library
add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})

# Include directories (explicitly specified)
target_include_directories(${ProjectName} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/.. 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../kernel
)

# Link Libraries (explicitly specified)
target_link_libraries(${ProjectName} PRIVATE 
    ${COIN3D_LIBRARY} 
    ${SOQT_LIBRARY}
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets
    TonatiuhKernel
)

# Set plugin output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/random
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/random
)

# Add install rules for all targets
install(TARGETS ${ProjectName}
    RUNTIME DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    LIBRARY DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    ARCHIVE DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
)