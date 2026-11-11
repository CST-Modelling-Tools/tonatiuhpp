cmake_minimum_required(VERSION 3.28)
set(ProjectName TransmissivityATMParameters)

project(${ProjectName})

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#Header files
set(HEADERS
    TransmissivityATMParameters.h
    TransmissivityATMParametersFactory.h
)

# Source files
set(SOURCES 
    TransmissivityATMParameters.cpp
    TransmissivityATMParametersFactory.cpp
)

# Resource files, if any
set(RESOURCES TransmissivityATMParameters.qrc)

# Add the plugin as a library
add_library(${ProjectName} SHARED ${HEADERS} ${SOURCES} ${RESOURCES})

# Specify include directories for this target
target_include_directories(${ProjectName} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/.. 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libraries 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../kernel
)

# Add compile definitions
target_compile_definitions(${ProjectName} PRIVATE TONATIUH_KERNEL_EXPORT)

# Link libraries
target_link_libraries(${ProjectName} PRIVATE 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets 
    TonatiuhppKernel
    # Assuming COIN3D_LIBRARY and SOQT_LIBRARY are set in the top-level CMakeLists.txt
    ${COIN3D_LIBRARY}
    ${SOQT_LIBRARY}
)

# Specify the installation rules
#install(TARGETS ${ProjectName} 
#    LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins/air"
#    RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins/air"
#)

