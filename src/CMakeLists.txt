include_directories("${PROJECT_SOURCE_DIR}/include")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ./bin)
set(CMAKE_BUILD_TYPE Debug)

set(cpu_module_src cpu_interface_generic.c)
set(fan_module_src fan_interface_pigpiodif2_pipe.c)
set(sources
    ${cpu_module_src}
    ${fan_module_src}
    fand.c
)

add_executable(fand ${sources})
target_link_libraries(fand m inih rt pigpiod_if2)

install(TARGETS fand DESTINATION bin)
