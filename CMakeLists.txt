cmake_minimum_required(VERSION 3.10)

project(CalculatorPluginSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(calc
    src/main.cpp
    src/loader.cpp
    src/parser.cpp 
    src/evaluator.cpp)

target_sources(calc PRIVATE
    #include/parser.h
    include/loader.h
 include/parser.h include/evaluator.h)

target_include_directories(calc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(plugins)

add_custom_target(copy_plugins ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:calc>/plugins
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:funcsin>
            $<TARGET_FILE:funccos>
            $<TARGET_FILE:funcpow>
            $<TARGET_FILE_DIR:calc>/plugins/
    DEPENDS funcsin funccos funcpow
    COMMENT "Копирование плагинов в папку plugins..."
)