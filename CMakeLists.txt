cmake_minimum_required(VERSION 3.20)

project(
    Party
    VERSION 0.1.0
    LANGUAGES CXX
)

# ================================================
# C++
# ================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# ================================================
# RAYLIB
# ================================================

# Raylib viene instalado mediante MSYS2 UCRT64.
# No usamos rutas absolutas a ninguna PC.

find_package(
    raylib
    CONFIG
    REQUIRED
)


# ================================================
# ARCHIVOS DEL JUEGO
# ================================================

file(
    GLOB_RECURSE
    PARTY_SOURCES

    CONFIGURE_DEPENDS

    "${CMAKE_CURRENT_SOURCE_DIR}/Core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/UI/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Gameplay/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Minigames/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Systems/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Board/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Entities/*.cpp"
)

# Un backup que conserve extension .cpp no debe convertirse por accidente
# en una unidad de compilacion. Esto evita compilar archivos temporales como
# MinijuegoColorSeguro.conflicto.bak.cpp.
list(
    FILTER PARTY_SOURCES
    EXCLUDE REGEX
    ".*\\.bak\\.cpp$"
)

list(
    FILTER PARTY_SOURCES
    EXCLUDE REGEX
    ".*\\.conflicto\\..*\\.cpp$"
)


# ================================================
# EJECUTABLE
# ================================================

add_executable(
    Party

    main.cpp

    ${PARTY_SOURCES}
)


# ================================================
# INCLUDES
# ================================================

# Permite includes como:
#
# #include "Core/Juego.h"
# #include "Minigames/..."
# #include "UI/..."

target_include_directories(
    Party

    PRIVATE

    "${CMAKE_CURRENT_SOURCE_DIR}"
)


# ================================================
# LIBRERIAS
# ================================================

target_link_libraries(
    Party

    PRIVATE

    raylib
)


# ================================================
# SALIDA
# ================================================

set_target_properties(
    Party

    PROPERTIES

    RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}"
)
