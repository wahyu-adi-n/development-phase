cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(development-phase)

set(CMAKE_PREFIX_PATH "libtorch/share/cmake/Torch")
find_package(Torch REQUIRED)

add_executable(development-phase infer_model.cpp)
target_link_libraries(development-phase "${TORCH_LIBRARIES}")
set_property(TARGET development-phase PROPERTY CXX_STANDARD 14)