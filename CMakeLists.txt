cmake_minimum_required(VERSION 3.20)
project(Dual C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(
        box3d
        GIT_REPOSITORY https://github.com/erincatto/box3d.git
        GIT_TAG        main
)
FetchContent_MakeAvailable(box3d)

# =====================================================
# RESOURCES PATH
# =====================================================
add_compile_definitions(EXTERNAL_RESOURCES_PATH="${CMAKE_SOURCE_DIR}/Jeux/resources")

add_compile_definitions(INTERNAL_RESOURCES_PATH="${CMAKE_SOURCE_DIR}/SDK/internal_resources")
add_compile_definitions(INTERNAL_RESOURCES_SHADERS_PATH="${CMAKE_SOURCE_DIR}/SDK/internal_resources/shaders")

# ========================================================================
# 1. CONFIGURATION DU MOTEUR (L'exécutable principal)
# ========================================================================

# Liste des fichiers sources du moteur uniquement
set(MOTEUR_SOURCES
        SDK/src/DUAL_Core/dual_core.c
        SDK/thirdparty/glad/glad.c
        Jeux/Testing/main.c
        SDK/include/dual_internal.h
        SDK/src/DUAL_Input/dual_input.c
        SDK/src/DUAL_Math/dual_math.c
        SDK/src/DUAL_Graphics/dual_graphics_2d.c
        SDK/src/DUAL_Graphics/dual_graphics_3d.c
        SDK/src/DUAL_Resources/dual_resources.c
        SDK/src/DUAL_Audio/dual_audio.c
        SDK/src/DUAL_FS/dual_fs.c
        SDK/include/game_api.h
        SDK/include/dual_utils.h
        SDK/include/DUAL_Graphics/particle_system.h
        SDK/src/DUAL_Graphics/particle_system.c
)

# Sources du SDK partagées entre le moteur et les tests (sans aucun main.c)
set(SDK_SOURCES
        SDK/src/DUAL_Core/dual_core.c
        SDK/thirdparty/glad/glad.c
        SDK/include/dual_internal.h
        SDK/src/DUAL_Input/dual_input.c
        SDK/src/DUAL_Math/dual_math.c
        SDK/src/DUAL_Graphics/dual_graphics_2d.c
        SDK/src/DUAL_Graphics/dual_graphics_3d.c
        SDK/src/DUAL_Graphics/camera3d.c
        SDK/src/DUAL_Graphics/camera2d.c
        SDK/src/DUAL_Graphics/particle_system.c
        SDK/src/DUAL_Graphics/shader.c
        SDK/src/DUAL_Resources/dual_resources.c
        SDK/src/DUAL_Audio/dual_audio.c
        SDK/src/DUAL_FS/dual_fs.c
        SDK/include/game_api.h
        SDK/include/dual_utils.h
)

add_executable(Dual ${MOTEUR_SOURCES})

# Flag crucial pour que le moteur exporte ses fonctions vers la clé USB (dlopen)
target_link_options(Dual PRIVATE "-rdynamic")

target_compile_definitions(Dual PRIVATE ASSETS_DIR="Jeux/Jeu1/assets")

# Association des dossiers includes au moteur
target_include_directories(Dual PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/include
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/thirdparty
)

# Dépendances système du moteur
find_package(PkgConfig REQUIRED)
find_package(assimp REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPUS REQUIRED opus opusfile)
include_directories(${OPUS_INCLUDE_DIRS})

target_link_libraries(Dual PRIVATE ${GLFW_LIBRARIES} ${OPUS_LIBRARIES} OpenGL::GL assimp::assimp m pthread dl box3d)


# ========================================================================
# 2. CONFIGURATION DU JEU (La cartouche .so)
# ========================================================================


add_library(game SHARED Jeux/Jeu1/game.c)
target_include_directories(game PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/include
)
set_target_properties(game PROPERTIES PREFIX "")
set(CLE_USB "/media/killian/692B-8D17")
add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:game>
        ${CLE_USB}/game.so
        COMMENT "Copie de game.so vers la racine de la cle USB..."
)


# ========================================================================
# 3. GENERATION DU SDK DYNAMIQUE (libdual.so)
# ========================================================================

add_library(dual SHARED ${SDK_SOURCES})
target_include_directories(dual PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/include
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/thirdparty
)
target_link_libraries(dual PRIVATE ${GLFW_LIBRARIES} ${OPUS_LIBRARIES} OpenGL::GL assimp::assimp m pthread dl)


# ========================================================================
# TESTS
# ========================================================================

# Test core
add_executable(test_core Jeux/Testing/test_core.c ${SDK_SOURCES})
target_include_directories(test_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/include
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/thirdparty
)
target_link_libraries(test_core PRIVATE ${GLFW_LIBRARIES} OpenGL::GL assimp::assimp m pthread dl)

# Test resources
add_executable(test_resources Jeux/Testing/test_resources.c ${SDK_SOURCES})
target_include_directories(test_resources PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/include
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/thirdparty
)
target_link_libraries(test_resources PRIVATE ${GLFW_LIBRARIES} OpenGL::GL assimp::assimp m pthread dl)

# Test math
add_executable(test_math Jeux/Testing/test_math.c ${SDK_SOURCES})
target_include_directories(test_math PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/include
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/thirdparty
)
target_link_libraries(test_math PRIVATE ${GLFW_LIBRARIES} OpenGL::GL assimp::assimp m pthread dl)

# Test graphics 2d
add_executable(test_graphics_2d Jeux/Testing/test_graphics_2d.c ${SDK_SOURCES})
target_include_directories(test_graphics_2d PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/include
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/thirdparty
)
target_link_libraries(test_graphics_2d PRIVATE ${GLFW_LIBRARIES} OpenGL::GL assimp::assimp m pthread dl)

# Test graphics 3d
add_executable(test_graphics_3d Jeux/Testing/test_graphics_3d.c ${SDK_SOURCES})
target_include_directories(test_graphics_3d PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/include
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/thirdparty
)
target_link_libraries(test_graphics_3d PRIVATE ${GLFW_LIBRARIES} OpenGL::GL assimp::assimp m pthread dl)

# Test File Saver
add_executable(test_fs Jeux/Testing/test_fs.c ${SDK_SOURCES})
target_include_directories(test_fs PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/include
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/thirdparty
)
target_link_libraries(test_fs PRIVATE ${GLFW_LIBRARIES} OpenGL::GL assimp::assimp m pthread dl)

# Test Audio
add_executable(test_audio Jeux/Testing/test_audio.c ${SDK_SOURCES})
target_include_directories(test_audio PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/include
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/thirdparty
)
target_link_libraries(test_audio PRIVATE ${GLFW_LIBRARIES} OpenGL::GL assimp::assimp m pthread dl)

# Test Audio
add_executable(test_physics_3d Jeux/Testing/test_physics_3d.c ${SDK_SOURCES})
target_include_directories(test_physics_3d PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/include
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/thirdparty
)
target_link_libraries(test_physics_3d PRIVATE ${GLFW_LIBRARIES} OpenGL::GL assimp::assimp m pthread dl box3d)