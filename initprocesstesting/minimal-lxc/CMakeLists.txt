
cmake_minimum_required(VERSION 3.0.2)

project(container)

find_package(PkgConfig REQUIRED)

pkg_check_modules(lxc REQUIRED lxc>=2.0.0)

add_executable(container
    container.c
)

target_link_libraries(container
    ${lxc_LIBRARIES}
)

add_executable(init
    initprocess.c
)
