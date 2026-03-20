cmake_minimum_required(VERSION 3.19)
project(Museum LANGUAGES CXX)

# -------------------------
# Qt Setup
# -------------------------
find_package(Qt5 5.12 REQUIRED COMPONENTS Core Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (MSVC)
    add_compile_options(/W4)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra)
endif()

# -------------------------
# Sources
# -------------------------
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.cxx"
    "src/*.hpp"
    "src/*.h"
)

add_executable(Museum ${SOURCES})

# -------------------------
# ASAN (clang, gcc, msvc)
# -------------------------
message(STATUS "AddressSanitizer enabled")

if (MSVC)
    target_compile_options(Museum PRIVATE /fsanitize=address)
    # target_link_options(Museum PRIVATE /fsanitize=address)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(Museum PRIVATE
        -fsanitize=address
        -fno-omit-frame-pointer
    )
    target_link_options(Museum PRIVATE
        -fsanitize=address
    )
endif()

# -------------------------
# Link Qt
# -------------------------
target_link_libraries(Museum
    PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
)

if (WIN32)
    target_link_libraries(Museum PRIVATE Shlwapi)
endif()

set_property(
    TARGET Museum PROPERTY VS_DEBUGGER_COMMAND $<TARGET_FILE:Museum>
)

# -------------------------
# Install
# -------------------------
include(GNUInstallDirs)

install(TARGETS Museum
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if (WIN32)
    install(CODE "
        execute_process(
            COMMAND \"${QT_ROOT}/bin/windeployqt.exe\" \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/Museum.exe\"
        )
    ")
endif()
