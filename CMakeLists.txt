cmake_minimum_required(VERSION 3.16) # CMake_最低版本_需求

file(GLOB sources CONFIGURE_DEPENDS src/*.c src/*.cpp)

link_directories(lib)

set(NEED_LIBRARYS sodium png z)

project("Main") # 建立一個CMake專案，並給予專案名稱

add_executable(cry) # 建立一個CMake目標，目標為可執行檔

target_sources(cry PUBLIC ${sources}) # 指定建置該CMake目標時所使用的來源檔案

target_include_directories(cry PUBLIC "include")
target_link_libraries(cry PUBLIC ${NEED_LIBRARYS})

target_link_options(cry PUBLIC -static-libgcc -static-libstdc++)

set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")