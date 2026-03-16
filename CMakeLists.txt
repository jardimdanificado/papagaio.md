cmake_minimum_required(VERSION 3.10)
project(papagaio C)

set(CMAKE_C_STANDARD 11)

# Options
option(HAS_FFI "Enable FFI features (requires libffi)" OFF)
option(HAS_DL  "Enable Dynamic Loading features" ON)

# Lua Source
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lua/lua.c")
    message(STATUS "Lua source not found, please run 'make fetch_lua_if_needed' or clone it to ./lua")
endif()

# Build Lua as a library
file(GLOB LUA_CORE_SOURCES "lua/l*.c")
list(REMOVE_ITEM LUA_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/lua/lua.c" "${CMAKE_CURRENT_SOURCE_DIR}/lua/luac.c" "${CMAKE_CURRENT_SOURCE_DIR}/lua/onelua.c")
add_library(liblua STATIC ${LUA_CORE_SOURCES})
target_include_directories(liblua PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lua")
if(UNIX AND NOT APPLE)
    target_link_libraries(liblua INTERFACE m dl)
endif()

# Papagaio module
add_library(papagaio SHARED papagaio/papagaio.c)
target_link_libraries(papagaio PRIVATE liblua)
set_target_properties(papagaio PROPERTIES PREFIX "")

# Papagaio-FFI module
file(GLOB FFI_SRC "papagaio-ffi/papagaio_ffi.c")
add_library(papagaio_ffi SHARED ${FFI_SRC})
target_link_libraries(papagaio_ffi PRIVATE liblua)
set_target_properties(papagaio_ffi PROPERTIES PREFIX "")

if(HAS_DL)
    target_compile_definitions(papagaio_ffi PRIVATE PAPAGAIO_FFI_HAS_DL)
    if(UNIX AND NOT APPLE)
        target_link_libraries(papagaio_ffi PRIVATE dl)
    endif()
endif()

if(HAS_FFI)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBFFI REQUIRED libffi)
    target_compile_definitions(papagaio_ffi PRIVATE PAPAGAIO_FFI_HAS_FFI)
    target_include_directories(papagaio_ffi PRIVATE ${LIBFFI_INCLUDE_DIRS})
    target_link_libraries(papagaio_ffi PRIVATE ${LIBFFI_LIBRARIES})
endif()

# CLI
add_executable(papagaio-md 
    papagaio-md/cli.c 
    papagaio/papagaio.c 
    papagaio-ffi/papagaio_ffi.c
    papagaio-md/papagaio_md_script.c
)
target_include_directories(papagaio-md PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/lua"
    "${CMAKE_CURRENT_SOURCE_DIR}/papagaio"
    "${CMAKE_CURRENT_SOURCE_DIR}/papagaio-ffi"
)
target_link_libraries(papagaio-md PRIVATE liblua)
target_compile_definitions(papagaio-md PRIVATE PAPAGAIO_FFI_HAS_DL)
if(HAS_FFI)
    target_compile_definitions(papagaio-md PRIVATE PAPAGAIO_FFI_HAS_FFI)
    target_include_directories(papagaio-md PRIVATE ${LIBFFI_INCLUDE_DIRS})
    target_link_libraries(papagaio-md PRIVATE ${LIBFFI_LIBRARIES})
endif()
if(UNIX AND NOT APPLE)
    target_link_libraries(papagaio-md PRIVATE dl m)
endif()
