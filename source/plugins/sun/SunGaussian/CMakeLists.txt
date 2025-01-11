cmake_minimum_required(VERSION 3.28)
set(ProjectName SunGaussian)

project(${ProjectName})

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Header and Source files
set(HEADERS
    SunGaussian.h
)
set(SOURCES 
    SunGaussian.cpp
)
set(RESOURCES resources.qrc)

# Add the plugin as a shared library
add_library(${ProjectName} SHARED ${HEADERS} ${SOURCES} ${RESOURCES})

# Include directories
target_include_directories(${ProjectName} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/.. 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libraries 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../kernel
)

# Link Libraries
target_link_libraries(${ProjectName} PRIVATE 
    ${COIN3D_LIBRARY} 
    ${SOQT_LIBRARY}
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets 
    TonatiuhKernel
)

# Specify the plugin output directory
set_target_properties(${ProjectName} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/sun
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/sun
)

# Add install rules for all targets
install(TARGETS ${ProjectName}
    RUNTIME DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    LIBRARY DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    ARCHIVE DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
)