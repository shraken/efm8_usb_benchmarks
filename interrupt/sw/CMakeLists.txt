project( benchmark )
cmake_minimum_required(VERSION 2.8)

# header files
set (MAIN_INCLUDE ${PROJECT_SOURCE_DIR}) 
set (HIDAPI_INCLUDE ${PROJECT_SOURCE_DIR}/lib/hidapi/hidapi) 
set (BENCHMARK_INCLUDE_DIRS 
        ${MAIN_INCLUDE} 
        ${HIDAPI_INCLUDE}
)

set(HIDAPI_SOURCES ${PROJECT_SOURCE_DIR}/lib/hidapi/windows/hid.c)
set(EXTRA_LIBS ${IOKIT_LIBRARY} setupapi.lib)

set (BENCHMARK_SOURCES ${HIDAPI_SOURCES})

include_directories(${BENCHMARK_INCLUDE_DIRS})
add_executable (benchmark benchmark.c ${BENCHMARK_SOURCES})
target_link_libraries(benchmark ${EXTRA_LIBS})