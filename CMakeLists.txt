# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Early-evaluated flag to decide if we configure Pico SDK/toolchain
if(DEFINED BUILD_FIRMWARE)
    set(BUILD_FIRMWARE_EARLY ${BUILD_FIRMWARE})
else()
    set(BUILD_FIRMWARE_EARLY ON)
endif()

# Initialise pico_sdk from installed location only if building firmware
if(BUILD_FIRMWARE_EARLY)
    # == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
    if(WIN32)
        set(USERHOME $ENV{USERPROFILE})
    else()
        set(USERHOME $ENV{HOME})
    endif()
    set(sdkVersion 2.2.0)
    set(toolchainVersion 14_2_Rel1)
    set(picotoolVersion 2.2.0)
    set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
    if (EXISTS ${picoVscode})
        include(${picoVscode})
    endif()
    # ====================================================================================
    set(PICO_BOARD pico2 CACHE STRING "Board type")

    # Pull in Raspberry Pi Pico SDK (must be before project)
    include(pico_sdk_import.cmake)
endif()

project(rp2040_maze_solver C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
if(BUILD_FIRMWARE_EARLY)
    pico_sdk_init()
endif()

# Options to control which targets to build
option(BUILD_FIRMWARE "Build firmware for RP2040" ON)
option(BUILD_TESTS "Build Unity-based unit tests (host)" OFF)
option(BUILD_SIM "Build desktop simulator (host)" OFF)

# Add executable. Default name is the project name, version 0.1
if(BUILD_FIRMWARE)
    # Use only the H-Bridge motor controller implementation
    set(MOTOR_IMPL "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/h_bridge/MotorControl_HBridge.cpp" CACHE INTERNAL "Motor control implementation .cpp file")
    # Also set as normal cache with FORCE to override any previous user cache
    set(MOTOR_IMPL "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/h_bridge/MotorControl_HBridge.cpp" CACHE STRING "Motor control implementation .cpp file" FORCE)

    # Collect sources: include all core and firmware, but do NOT glob hal implementations
    file(GLOB_RECURSE FW_SOURCES_CORE
        ${CMAKE_CURRENT_LIST_DIR}/src/core/*.cpp
        ${CMAKE_CURRENT_LIST_DIR}/firmware/*.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/hal/IRSensorArray.cpp
    )

    add_executable(rp2040_maze_solver
        firmware/main.cpp
        ${MOTOR_IMPL}
        src/hal/IRSensorArray.cpp
        src/core/Navigator.cpp
        src/core/PersistentMemory.cpp
    )

    if(BUILD_FIRMWARE)
        pico_set_program_name(rp2040_maze_solver "rp2040_maze_solver")
        pico_set_program_version(rp2040_maze_solver "0.1")
    endif()

    if(BUILD_FIRMWARE)
        # Add any user requested libraries
        target_link_libraries(rp2040_maze_solver
            pico_stdlib
            hardware_adc
        )
    endif()

    # ------------------------------------------------------------
    # Control and sensing tuning parameters (compile-time overrides)
    # 
    # You can override any of these from command line, e.g.:
    #   cmake -B build -S . \
    #     -DCONTROL_PERIOD_MS=150 \
    #     -DIR_ALPHA=0.23 \
    #     -DIR_TH_FREE=0.55 \
    #     -DIR_TH_NEAR=0.30 \
    #     -DK_ROT=1.2 \
    #     -DFWD_BASE=0.35 \
    #     -DTURN_FWD=0.15 \
    #     -DTURN_ROT=0.7
    # ------------------------------------------------------------
    set(CONTROL_PERIOD_MS 150 CACHE STRING "Control loop period in ms")
    set(IR_ALPHA 0.23 CACHE STRING "EMA alpha for IR filter")
    set(IR_TH_FREE 0.55 CACHE STRING "IR threshold for free path")
    set(IR_TH_NEAR 0.30 CACHE STRING "IR threshold considered near obstacle")
    set(K_ROT 1.2 CACHE STRING "Rotation gain for proportional centering")
    set(FWD_BASE 0.35 CACHE STRING "Base forward command targeting ~5 cm/s")
    set(TURN_FWD 0.15 CACHE STRING "Forward component when turning in place/entry")
    set(TURN_ROT 0.7 CACHE STRING "Rotation magnitude when turning left/right")

    # Physical dimensions and targets
    set(ROBOT_WIDTH_CM 15.0 CACHE STRING "Robot width in cm")
    set(ROBOT_LENGTH_CM 15.0 CACHE STRING "Robot length in cm")
    set(ENTRY_WIDTH_CM 20.0 CACHE STRING "Entry (door) width in cm")
    set(TARGET_SPEED_CM_S 5.0 CACHE STRING "Target cruising speed in cm/s")

    # Persistence (flash) configuration
    # Total flash size in bytes (used to place persistence sector at the end of flash)
    # Override based on your board (e.g., 4MB = 4194304)
    set(PMEM_FLASH_TOTAL_BYTES 2097152 CACHE STRING "Total flash size in bytes for persistence calculations")

    # Default pin mapping (override per board/wiring)
    set(MOTOR_L_PWM 0 CACHE STRING "GPIO for left motor PWM (IN1)")
    set(MOTOR_L_DIRA 1 CACHE STRING "GPIO for left motor DIR (IN2)")
    set(MOTOR_L_DIRB 2 CACHE STRING "GPIO for left motor DIRB (unused for H-bridge)")
    set(MOTOR_R_PWM 3 CACHE STRING "GPIO for right motor PWM (IN1)")
    set(MOTOR_R_DIRA 4 CACHE STRING "GPIO for right motor DIR (IN2)")
    set(MOTOR_R_DIRB 5 CACHE STRING "GPIO for right motor DIRB (unused for H-bridge)")

    set(IR_ADC_LEFT 0 CACHE STRING "ADC channel index for left IR (0..4)")
    set(IR_ADC_FRONT 1 CACHE STRING "ADC channel index for front IR (0..4)")
    set(IR_ADC_RIGHT 2 CACHE STRING "ADC channel index for right IR (0..4)")

    # Optional geometry-based auto-tuning flag
    set(AUTO_TUNE_GEOM 1 CACHE STRING "Enable geometry-based gain scaling (1 on, 0 off)")

    target_compile_definitions(rp2040_maze_solver PRIVATE
        CFG_CONTROL_PERIOD_MS=${CONTROL_PERIOD_MS}
        CFG_IR_ALPHA=${IR_ALPHA}
        CFG_IR_TH_FREE=${IR_TH_FREE}
        CFG_IR_TH_NEAR=${IR_TH_NEAR}
        CFG_K_ROT=${K_ROT}
        CFG_FWD_BASE=${FWD_BASE}
        CFG_TURN_FWD=${TURN_FWD}
        CFG_TURN_ROT=${TURN_ROT}
        CFG_ROBOT_WIDTH_CM=${ROBOT_WIDTH_CM}
        CFG_ROBOT_LENGTH_CM=${ROBOT_LENGTH_CM}
        CFG_ENTRY_WIDTH_CM=${ENTRY_WIDTH_CM}
        CFG_TARGET_SPEED_CM_S=${TARGET_SPEED_CM_S}
        CFG_AUTO_TUNE_GEOM=${AUTO_TUNE_GEOM}
        CFG_MOTOR_L_PWM=${MOTOR_L_PWM}
        CFG_MOTOR_L_DIRA=${MOTOR_L_DIRA}
        CFG_MOTOR_L_DIRB=${MOTOR_L_DIRB}
        CFG_MOTOR_R_PWM=${MOTOR_R_PWM}
        CFG_MOTOR_R_DIRA=${MOTOR_R_DIRA}
        CFG_MOTOR_R_DIRB=${MOTOR_R_DIRB}
        CFG_IR_ADC_LEFT=${IR_ADC_LEFT}
        CFG_IR_ADC_FRONT=${IR_ADC_FRONT}
        CFG_IR_ADC_RIGHT=${IR_ADC_RIGHT}
        PMEM_FLASH_TOTAL_BYTES=${PMEM_FLASH_TOTAL_BYTES}
    )
endif()

if(BUILD_FIRMWARE)
    # Modify the below lines to enable/disable output over UART/USB
    pico_enable_stdio_uart(rp2040_maze_solver 0)
    pico_enable_stdio_usb(rp2040_maze_solver 1)
endif()

if(BUILD_FIRMWARE)
    # Add the standard library to the build
    target_link_libraries(rp2040_maze_solver
            pico_stdlib)
endif()

if(BUILD_FIRMWARE)
    # Add the standard include files to the build
    target_include_directories(rp2040_maze_solver PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}
            ${CMAKE_CURRENT_LIST_DIR}/src
            ${CMAKE_CURRENT_LIST_DIR}/firmware
    )
endif()

if(BUILD_FIRMWARE)
    # Add any user requested libraries
    target_link_libraries(rp2040_maze_solver 
            hardware_i2c
            hardware_timer
            hardware_clocks
            hardware_pwm
            hardware_adc
            hardware_flash
    )
endif()

if(BUILD_FIRMWARE)
    pico_add_extra_outputs(rp2040_maze_solver)
endif()

# ------------------------------
# Host-side tests using Unity
if(BUILD_TESTS)
    enable_testing()
    add_executable(maze_tests
        src/core/Navigator.cpp
        src/core/PersistentMemory.cpp
        tests/test_navigator.cpp
        inc/Unity/src/unity.c
    )
    target_include_directories(maze_tests PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/inc/Unity/src
    )
    add_test(NAME navigator_right_hand COMMAND maze_tests)

    add_executable(planner_tests
        tests/test_planner.cpp
        inc/Unity/src/unity.c
    )
    target_include_directories(planner_tests PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/inc/Unity/src
    )
    add_test(NAME planner_bfs COMMAND planner_tests)

    add_executable(navigator_planned_tests
        tests/test_navigator_planned.cpp
        src/core/Navigator.cpp
        src/core/PersistentMemory.cpp
        inc/Unity/src/unity.c
    )
    target_include_directories(navigator_planned_tests PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/inc/Unity/src
    )
    add_test(NAME navigator_planned COMMAND navigator_planned_tests)

    # Random maze tests (BFS should find a path in perfect mazes)
    add_executable(random_maze_tests
        tests/test_random_mazes.cpp
        inc/Unity/src/unity.c
    )
    target_include_directories(random_maze_tests PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/inc/Unity/src
    )
    add_test(NAME random_mazes COMMAND random_maze_tests)

    # Learning tests (second episode should be <= first in cost)
    add_executable(learning_tests
        tests/test_learning.cpp
        src/core/Navigator.cpp
        inc/Unity/src/unity.c
    )
    target_include_directories(learning_tests PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/inc/Unity/src
    )
    add_test(NAME learning COMMAND learning_tests)

    # Reach-goal tests: ensure agent reaches goal in random mazes
    add_executable(reach_goal_tests
        tests/test_reach_goal.cpp
        src/core/Navigator.cpp
        inc/Unity/src/unity.c
    )
    target_include_directories(reach_goal_tests PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/inc/Unity/src
    )
    add_test(NAME reach_goal COMMAND reach_goal_tests)
    
    # Map snapshot persistence tests
    add_executable(persistence_map_tests
        tests/test_persistence_map.cpp
        src/core/PersistentMemory.cpp
        inc/Unity/src/unity.c
    )
    target_include_directories(persistence_map_tests PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/inc/Unity/src
    )
    add_test(NAME persistence_map COMMAND persistence_map_tests)
endif()

# ------------------------------
# Optional SDL2-based simulator (very simple GUI)
if(BUILD_SIM)
    find_package(SDL2 QUIET)
    # Try common Find-module name first
    find_package(SDL2_ttf QUIET)
    # Some systems ship an upper-case variant
    if(NOT SDL2_TTF_FOUND)
        find_package(SDL2_TTF QUIET)
    endif()
    # Fallback via pkg-config if still not found
    if(NOT SDL2_TTF_FOUND)
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            # Correct pkg-config name is 'SDL2_ttf'
            pkg_check_modules(PC_SDL2_TTF QUIET SDL2_ttf)
            if(PC_SDL2_TTF_FOUND)
                set(SDL2_TTF_FOUND TRUE)
                set(SDL2_TTF_INCLUDE_DIRS ${PC_SDL2_TTF_INCLUDE_DIRS})
                set(SDL2_TTF_LIBRARIES ${PC_SDL2_TTF_LIBRARIES})
            endif()
        endif()
    endif()
    # If includes are known but libs are empty, try find_library
    if(SDL2_TTF_FOUND AND (NOT SDL2_TTF_LIBRARIES OR SDL2_TTF_LIBRARIES STREQUAL ""))
        find_library(SDL2_TTF_LIBRARY SDL2_ttf)
        if(SDL2_TTF_LIBRARY)
            set(SDL2_TTF_LIBRARIES ${SDL2_TTF_LIBRARY})
        else()
            unset(SDL2_TTF_FOUND)
        endif()
    endif()
    if(SDL2_FOUND)
        add_executable(simulator
            simulator/main.cpp
            src/core/Navigator.cpp
        )
        target_include_directories(simulator PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/src
            ${CMAKE_CURRENT_LIST_DIR}/inc
            ${SDL2_INCLUDE_DIRS}
        )
        target_link_libraries(simulator PRIVATE ${SDL2_LIBRARIES})
        if(SDL2_TTF_FOUND AND SDL2_TTF_LIBRARIES)
            target_include_directories(simulator PRIVATE ${SDL2_TTF_INCLUDE_DIRS})
            target_link_libraries(simulator PRIVATE ${SDL2_TTF_LIBRARIES})
            target_compile_definitions(simulator PRIVATE HAVE_SDL_TTF=1)
            message(STATUS "SDL2_ttf found: libs=${SDL2_TTF_LIBRARIES}")
        else()
            message(STATUS "SDL2_ttf not found; building simulator without text rendering. Install libsdl2-ttf-dev for labels.")
        endif()
    else()
        message(STATUS "SDL2 not found; simulator target will not be built.")
    endif()
endif()

# ------------------------------
# Optional documentation (Doxygen)
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
  set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
  if(EXISTS ${DOXYFILE_IN})
    add_custom_target(docs
      COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_IN}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM)
  else()
    set(DOXYGEN_PROJECT_NAME "RP2040 Maze Solver")
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_HTML_OUTPUT .)
    set(DOXYGEN_RECURSIVE YES)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/inc ${CMAKE_CURRENT_SOURCE_DIR}/firmware ${CMAKE_CURRENT_SOURCE_DIR}/simulator ${CMAKE_CURRENT_SOURCE_DIR}/README.md ${CMAKE_CURRENT_SOURCE_DIR}/PLAN.md ${CMAKE_CURRENT_SOURCE_DIR}/REFERENCE.md")
    doxygen_add_docs(docs ${DOXYGEN_INPUT})
  endif()
else()
  message(STATUS "Doxygen not found; 'docs' target will not be available.")
endif()
