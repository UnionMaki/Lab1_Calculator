function(add_plugin name src)
    add_library(${name} MODULE ${src})
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/include)
endfunction()

add_plugin(funcsin  funcsin.cpp)
add_plugin(funccos  funccos.cpp)
add_plugin(funcpow  funcpow.cpp)