cmake_minimum_required(VERSION 2.8)

#add_subdirectory(Stockfish)

project(tiny_chess)

set(CMAKE_CXX_STANDARD 11)
add_definitions(-Wall -Wcast-qual -Wextra -Wshadow -fno-exceptions -fno-rtti -pedantic -flto -m64)

include_directories(src)
if (INCLUDE_STOCKFISH)
	include_directories(Stockfish/src)
endif()

file(GLOB_RECURSE sourceFiles ./src/*.cpp ./src/*.c)
file(GLOB_RECURSE headerFiles ./src/*.h)

if (INCLUDE_STOCKFISH)
#file(GLOB_RECURSE sourceFiles ./src/*.cpp ./src/*.c ./Stockfish/src/*.cpp ./Stockfish/src/*.c)
#file(GLOB_RECURSE headerFiles ./src/*.h ./Stockfish/src/*.h)
endif()


if (EMSCRIPTEN)
	set(CMAKE_EXECUTABLE_SUFFIX ".html")

#	set(linkFlags "-s DISABLE_EXCEPTION_CATCHING=1 -s TOTAL_MEMORY=2032MB --shell-file ${CMAKE_CURRENT_LIST_DIR}/src/tiny_chess_shell.html --js-library ${CMAKE_CURRENT_LIST_DIR}/src/library_unicode.js")
#	set(linkFlags "${linkFlags} -s USE_PTHREADS=1 -s DISABLE_EXCEPTION_CATCHING=1 -s TOTAL_MEMORY=2032MB -s PTHREAD_POOL_SIZE=4 -s ASMFS=1 -s PROXY_TO_PTHREAD=1 -s OFFSCREENCANVAS_SUPPORT=0 -s OFFSCREEN_FRAMEBUFFER=1 --threadprofiler --profiling-funcs")

	set(linkFlags "-s DISABLE_EXCEPTION_CATCHING=1 -s TOTAL_MEMORY=16MB --shell-file ${CMAKE_CURRENT_LIST_DIR}/src/tiny_chess_shell.html --js-library ${CMAKE_CURRENT_LIST_DIR}/src/library_unicode.js -s WASM=1")
	set(linkFlags "${linkFlags} -s DISABLE_EXCEPTION_CATCHING=1 -s TOTAL_MEMORY=16MB")


#	set(linkFlagsDebug "-s GL_DEBUG=1 -g2")
endif()

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

#set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${linkFlagsDebug}") #TODO why does this not work?
set(CMAKE_EXE_LINKER_FLAGS "${linkFlagsDebug} ${linkFlags}")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${linkFlagsDebug} ${linkFlags}")

add_executable(tiny_chess ${sourceFiles} ${headerFiles})

set_target_properties(tiny_chess PROPERTIES LINK_FLAGS_DEBUG "${linkFlagsDebug} ${linkFlags}")
set_target_properties(tiny_chess PROPERTIES LINK_FLAGS_RELEASE "${linkFlags}")

#target_link_libraries(tiny_chess Stockfish)
