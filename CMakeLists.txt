cmake_minimum_required(VERSION 3.20)

project(VampireCrys VERSION 0.1.0 LANGUAGES C CXX)

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib 5.5 QUIET)

if(NOT raylib_FOUND)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(raylib)
endif()

find_package(nlohmann_json 3.11 QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

add_executable(VampireCrys
    src/main.cpp
    src/core/Application.cpp
    src/core/GameSettings.cpp
    src/entities/Player.cpp
    src/game_modes/GameModeManager.cpp
    src/game_modes/ExpeditionMap.cpp
    src/game_modes/ExpeditionDirector.cpp
    src/game_modes/ExpeditionMode.cpp
    src/game_modes/SurvivalMode.cpp
    src/game_modes/SurvivalEvents.cpp
    src/game_modes/SurvivalEndgame.cpp
    src/gameplay/Definitions.cpp
    src/gameplay/Characters.cpp
    src/gameplay/GameSession.cpp
    src/systems/AreaEffectManager.cpp
    src/systems/AudioManager.cpp
    src/systems/BossManager.cpp
    src/systems/ChestManager.cpp
    src/systems/EnemyManager.cpp
    src/systems/FloatingTextManager.cpp
    src/systems/LootSystem.cpp
    src/systems/ParticleManager.cpp
    src/systems/PickupManager.cpp
    src/systems/ProjectileManager.cpp
    src/systems/ScreenShakeManager.cpp
    src/systems/SpatialGrid.cpp
    src/systems/TelegraphManager.cpp
    src/systems/WeaponManager.cpp
    src/systems/WaveDirector.cpp
    src/systems/XPOrbManager.cpp
    src/ui/GameUI.cpp
    src/ui/Localization.cpp
    src/ui/VisualStyle.cpp
)

target_include_directories(VampireCrys PRIVATE include)
target_link_libraries(VampireCrys PRIVATE raylib nlohmann_json::nlohmann_json)
target_compile_definitions(VampireCrys PRIVATE
    VAMPIRECRYS_VERSION="${PROJECT_VERSION}"
    $<$<CONFIG:Debug>:GAME_DEBUG>
)

if(MSVC)
    target_compile_options(VampireCrys PRIVATE /W4 /permissive- /utf-8 /FS)
else()
    target_compile_options(VampireCrys PRIVATE -Wall -Wextra -Wpedantic)
    if(ENABLE_SANITIZERS)
        target_compile_options(VampireCrys PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(VampireCrys PRIVATE -fsanitize=address,undefined)
    endif()
endif()

set_target_properties(VampireCrys PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

include(CTest)
if(BUILD_TESTING)
    add_executable(VampireCrysTests
        tests/SystemsTests.cpp
        src/core/GameSettings.cpp
        src/entities/Player.cpp
        src/game_modes/GameModeManager.cpp
        src/game_modes/ExpeditionMap.cpp
        src/game_modes/ExpeditionDirector.cpp
        src/game_modes/ExpeditionMode.cpp
        src/game_modes/SurvivalMode.cpp
        src/game_modes/SurvivalEvents.cpp
        src/game_modes/SurvivalEndgame.cpp
        src/gameplay/Definitions.cpp
        src/gameplay/Characters.cpp
        src/gameplay/GameSession.cpp
        src/systems/AreaEffectManager.cpp
        src/systems/AudioManager.cpp
        src/systems/BossManager.cpp
        src/systems/ChestManager.cpp
        src/systems/EnemyManager.cpp
        src/systems/FloatingTextManager.cpp
        src/systems/LootSystem.cpp
        src/systems/ParticleManager.cpp
        src/systems/PickupManager.cpp
        src/systems/ProjectileManager.cpp
        src/systems/ScreenShakeManager.cpp
        src/systems/SpatialGrid.cpp
        src/systems/TelegraphManager.cpp
        src/systems/WeaponManager.cpp
        src/systems/WaveDirector.cpp
        src/systems/XPOrbManager.cpp
        src/ui/VisualStyle.cpp
        src/ui/Localization.cpp
    )
    target_include_directories(VampireCrysTests PRIVATE include)
    target_link_libraries(VampireCrysTests PRIVATE raylib nlohmann_json::nlohmann_json)
    target_compile_definitions(VampireCrysTests PRIVATE VAMPIRECRYS_VERSION="${PROJECT_VERSION}")
    if(MSVC)
        target_compile_options(VampireCrysTests PRIVATE /W4 /permissive- /utf-8 /FS)
        # The intentionally broad single-file regression suite owns many fixed-size pool
        # fixtures. Give the test process room without changing the game's stack budget.
        target_link_options(VampireCrysTests PRIVATE /STACK:8388608)
    else()
        target_compile_options(VampireCrysTests PRIVATE -Wall -Wextra -Wpedantic)
        if(ENABLE_SANITIZERS)
            target_compile_options(VampireCrysTests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(VampireCrysTests PRIVATE -fsanitize=address,undefined)
        endif()
    endif()
    add_test(NAME systems COMMAND VampireCrysTests)
endif()

install(TARGETS VampireCrys RUNTIME DESTINATION . COMPONENT Runtime)
install(DIRECTORY config assets DESTINATION . COMPONENT Runtime)
install(FILES README.md ARCHITECTURE.md DECISIONS.md GAME_MODES.md ASSETS.md
              BALANCING.md PERFORMANCE.md FINAL_AUDIT.md CONTENT_CATALOG.md
              SURVIVAL_ENDGAME.md
              EXPEDITION.md
        DESTINATION . COMPONENT Runtime)
install(FILES RELEASE_README.txt DESTINATION . RENAME README.txt COMPONENT Runtime)

add_custom_command(TARGET VampireCrys POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/config" "$<TARGET_FILE_DIR:VampireCrys>/config"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:VampireCrys>/assets")
if(BUILD_TESTING)
    add_custom_command(TARGET VampireCrysTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/config" "$<TARGET_FILE_DIR:VampireCrysTests>/config")
endif()
