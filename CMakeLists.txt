cmake_minimum_required(VERSION 3.10)
project(TrabalhoGrafos VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Otimizações de performance para os grafos pesados da Parte 2
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

include_directories(include)

# Biblioteca estática com a implementação do CSR e algoritmos de grafos
add_library(grafos_lib
    src/CSRGraph.cpp
    src/algorithms.cpp
)

# Executável principal para os estudos de caso dos grafos W e rede de coautoria
add_executable(grafo src/main.cpp)
target_link_libraries(grafo PRIVATE grafos_lib)

# Executável da suíte de testes unitários (totalmente em memória)
add_executable(test_grafo tests/test_grafo.cpp)
target_link_libraries(test_grafo PRIVATE grafos_lib)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(test_grafo PRIVATE -UNDEBUG)
elseif(MSVC)
    target_compile_options(test_grafo PRIVATE /UNDEBUG)
endif()

enable_testing()
add_test(NAME test_grafo COMMAND test_grafo)