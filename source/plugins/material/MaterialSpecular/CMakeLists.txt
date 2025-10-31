cmake_minimum_required(VERSION 3.28)
set(ProjectName MaterialSpecular)

project(${ProjectName})

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Header and Source files
set(HEADERS
    MaterialSpecular.h
)
set(SOURCES 
    MaterialSpecular.cpp
)

# Resource files, if any
set(RESOURCES resources.qrc)

# Add the plugin as a shared library
add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES} ${RESOURCES})

# Include directories (explicitly specified)
target_include_directories(${ProjectName} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/.. 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../kernel
)

# Link libraries using global variables
target_link_libraries(${PROJECT_NAME} PRIVATE
    Coin::Coin
    SoQt::SoQt
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets 
    TonatiuhLibraries
    TonatiuhKernel
)

# Set plugin output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/material
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/material
)

# Add install rules for all targets
install(TARGETS ${ProjectName}
    RUNTIME DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    LIBRARY DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    ARCHIVE DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
)