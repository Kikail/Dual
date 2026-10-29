cmake_minimum_required(VERSION 3.20)
project(Dual C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Disable FetchContent updates")

FetchContent_Declare(
        box3d
        GIT_REPOSITORY https://github.com/erincatto/box3d.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(box3d)

# =====================================================
# RESOURCES PATH
# =====================================================
add_compile_definitions(EXTERNAL_RESOURCES_PATH="${CMAKE_SOURCE_DIR}/Jeux/resources")
add_compile_definitions(INTERNAL_RESOURCES_PATH="${CMAKE_SOURCE_DIR}/SDK/internal_resources")
add_compile_definitions(INTERNAL_RESOURCES_SHADERS_PATH="${CMAKE_SOURCE_DIR}/SDK/internal_resources/shaders")

# =====================================================
# DEPENDANCES SYSTEME (PkgConfig & FindPackages)
# =====================================================
find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0)
if(NOT GLIB_FOUND)
    message(FATAL_ERROR "GLib 2.0 est requis pour les GList et GHashTable. Installe-le via : sudo apt install libglib2.0-dev")
endif()

pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(OPUS REQUIRED opus opusfile)
find_package(assimp REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(${OPUS_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})

# ========================================================================
# 1. CONFIGURATION DU MOTEUR (L'exécutable principal)
# ========================================================================

set(MOTEUR_SOURCES
        Jeux/Testing/main.c
        SDK/src/DUAL_Core/dual_core.c
        SDK/thirdparty/glad/glad.c
        SDK/include/dual_internal.h
        SDK/src/DUAL_Input/dual_input.c
        SDK/src/DUAL_Math/dual_math.c
        SDK/src/DUAL_Graphics/dual_graphics_2d.c
        SDK/src/DUAL_Graphics/dual_graphics_3d.c
        SDK/src/DUAL_Physics/dual_physics.c
        SDK/src/DUAL_Graphics/camera3d.c
        SDK/src/DUAL_Graphics/camera2d.c
        SDK/src/DUAL_Graphics/particle_system.c
        SDK/src/DUAL_Graphics/shader.c
        SDK/src/DUAL_Graphics/dual_ui.c
        SDK/src/DUAL_Resources/dual_resources.c
        SDK/src/DUAL_Audio/dual_audio.c
        SDK/src/DUAL_FS/dual_fs.c
        SDK/src/DUAL_Editor/dual_scene_loader.c
        SDK/include/game_api.h
        SDK/include/dual_utils.h
)

set(SDK_SOURCES
        SDK/src/DUAL_Core/dual_core.c
        SDK/thirdparty/glad/glad.c
        SDK/include/dual_internal.h
        SDK/src/DUAL_Input/dual_input.c
        SDK/src/DUAL_Math/dual_math.c
        SDK/src/DUAL_Graphics/dual_graphics_2d.c
        SDK/src/DUAL_Graphics/dual_graphics_3d.c
        SDK/src/DUAL_Physics/dual_physics.c
        SDK/src/DUAL_Graphics/camera3d.c
        SDK/src/DUAL_Graphics/camera2d.c
        SDK/src/DUAL_Graphics/particle_system.c
        SDK/src/DUAL_Graphics/shader.c
        SDK/src/DUAL_Graphics/dual_ui.c
        SDK/src/DUAL_Resources/dual_resources.c
        SDK/src/DUAL_Audio/dual_audio.c
        SDK/src/DUAL_FS/dual_fs.c
        SDK/src/DUAL_Editor/dual_scene_loader.c
        SDK/include/game_api.h
        SDK/include/dual_utils.h
)

add_executable(Dual ${MOTEUR_SOURCES})

target_link_options(Dual PRIVATE "-rdynamic")
target_compile_definitions(Dual PRIVATE ASSETS_DIR="Jeux/Jeu1/assets")

target_include_directories(Dual PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/include
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/thirdparty
)

target_link_libraries(Dual PRIVATE ${GLFW_LIBRARIES} ${OPUS_LIBRARIES} ${GLIB_LIBRARIES} OpenGL::GL assimp::assimp m pthread dl box3d)


# ========================================================================
# 2. CONFIGURATION DU JEU (La cartouche .so)
# ========================================================================

add_library(game SHARED Jeux/Jeu2/game.c)
target_include_directories(game PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/include
)
target_link_libraries(game PRIVATE ${GLIB_LIBRARIES})
set_target_properties(game PROPERTIES PREFIX "")
set(CLE_USB "/home/killian/Desktop/Games/DUAL_FPS_Game")
add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:game>
        ${CLE_USB}/game.so
        COMMENT "Copie de game.so vers /home/killian/Desktop/Games/DUAL_FPS_Game ..."
)


# ========================================================================
# 3. GENERATION DU SDK DYNAMIQUE (libdual.so)
# ========================================================================

add_library(dual SHARED ${SDK_SOURCES})
target_include_directories(dual PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/include
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/thirdparty
)
target_link_libraries(dual PRIVATE ${GLFW_LIBRARIES} ${OPUS_LIBRARIES} ${GLIB_LIBRARIES} OpenGL::GL assimp::assimp m pthread dl)


# ========================================================================
# TESTS
# ========================================================================

macro(add_dual_test test_name source_file)
    add_executable(${test_name} ${source_file} ${SDK_SOURCES})
    target_include_directories(${test_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/SDK/include
            ${CMAKE_CURRENT_SOURCE_DIR}/SDK/thirdparty
    )
    target_link_libraries(${test_name} PRIVATE ${GLFW_LIBRARIES} ${GLIB_LIBRARIES} OpenGL::GL assimp::assimp m pthread dl box3d)
endmacro()

add_dual_test(test_core Jeux/Testing/test_core.c)
add_dual_test(test_resources Jeux/Testing/test_resources.c)
add_dual_test(test_math Jeux/Testing/test_math.c)
add_dual_test(test_ui Jeux/Testing/test_ui.c)
add_dual_test(test_graphics_2d Jeux/Testing/test_graphics_2d.c)
add_dual_test(test_graphics_3d Jeux/Testing/test_graphics_3d.c)
add_dual_test(test_fs Jeux/Testing/test_fs.c)
add_dual_test(test_audio Jeux/Testing/test_audio.c)
add_dual_test(test_physics_3d Jeux/Testing/test_physics_3d.c)
add_dual_test(testing_scene_loader Jeux/Testing/testing_scene_loader.c)
add_dual_test(fps_game_demo Jeux/Testing/fps_game_demo.c)