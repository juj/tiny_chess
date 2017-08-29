cmake_minimum_required(VERSION 2.8)

project(tiny_chess)

include_directories(src)

file(GLOB_RECURSE sourceFiles ./src/*.cpp ./src/*.c)
file(GLOB_RECURSE headerFiles ./src/*.h)

if (EMSCRIPTEN)
	set(CMAKE_EXECUTABLE_SUFFIX ".html")
	set(linkFlags "-s DISABLE_EXCEPTION_CATCHING=1 -s TOTAL_MEMORY=512MB --shell-file ${CMAKE_CURRENT_LIST_DIR}/src/tiny_chess_shell.html --js-library ${CMAKE_CURRENT_LIST_DIR}/src/library_unicode.js -s WASM=1")
#	set(linkFlags "-s USE_PTHREADS=1 -s DISABLE_EXCEPTION_CATCHING=1 -s TOTAL_MEMORY=512MB -g2 -s PTHREAD_POOL_SIZE=8")

	set(linkFlagsDebug "-s GL_DEBUG=1 -g2")
endif()

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

#set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${linkFlagsDebug}") #TODO why does this not work?
#set(CMAKE_EXE_LINKER_FLAGS "${linkFlagsDebug} ${linkFlags}")

add_executable(tiny_chess ${sourceFiles} ${headerFiles})

set_target_properties(tiny_chess PROPERTIES LINK_FLAGS_DEBUG "${linkFlagsDebug} ${linkFlags}")
set_target_properties(tiny_chess PROPERTIES LINK_FLAGS_RELEASE "${linkFlags}")
