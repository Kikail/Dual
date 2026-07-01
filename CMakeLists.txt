cmake_minimum_required(VERSION 3.20)
project(Dual C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 1. Liste de tes fichiers sources
set(SOURCES
        SDK/src/dual_core.c
        SDK/thirdparty/glad/glad.c
        Jeux/Testing/main.c
        SDK/include/dual_internal.h
        SDK/src/dual_input.c
        SDK/src/dual_math.c
        SDK/src/dual_graphics_2d.c
)

# 2. Création de l'exécutable
add_executable(Dual ${SOURCES})

# 3. Association DIRECTE et absolue du dossier include à l'exécutable Dual
target_include_directories(Dual PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/include
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/thirdparty
)

# 4. Dépendances système
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
find_package(OpenGL REQUIRED)

target_link_libraries(Dual PRIVATE ${GLFW_LIBRARIES} OpenGL::GL m pthread dl)