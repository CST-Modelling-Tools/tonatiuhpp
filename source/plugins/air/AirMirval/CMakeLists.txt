cmake_minimum_required(VERSION 3.28)
set(ProjectName AirMirval)

project(${ProjectName})

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Header and Source files
set(HEADERS
    AirMirval.h
)
set(SOURCES 
    AirMirval.cpp
)
set(RESOURCES resources.qrc)

# Add the plugin as a shared library
add_library(${ProjectName} SHARED ${HEADERS} ${SOURCES} ${RESOURCES})

# Include directories (explicitly specified)
target_include_directories(${ProjectName} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/.. 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../kernel
)

# Link Libraries (explicitly specified)
target_link_libraries(${ProjectName} PRIVATE 
    Coin::Coin
    SoQt::SoQt
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets
    TonatiuhKernel
)

# Specify the plugin output directory
set_target_properties(${ProjectName} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/air
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/air
)

# Add install rules for all targets
install(TARGETS ${ProjectName}
    RUNTIME DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    LIBRARY DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
    ARCHIVE DESTINATION "${GLOBAL_INSTALL_BIN_DIR}"
)
