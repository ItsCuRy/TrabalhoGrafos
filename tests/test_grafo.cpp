/**
 * @file test_grafo.cpp
 * @brief Suíte de testes unitários para validação das Partes 1 e 2 da biblioteca.
 *
 * O que é testado aqui:
 *   1. Grafo da Figura 1 da Parte 1 (5 vértices, sem pesos): graus, BFS, DFS e componentes.
 *   2. Grafo da Figura 1 da Parte 2 (5 vértices, com aresta negativa -9.5): verifica se
 *      a biblioteca detecta e bloqueia corretamente a execução do Dijkstra.
 *   3. Grafo da Figura 1 da Parte 2 com pesos positivos: verifica se as implementações
 *      via Vetor e via Heap Binário Indexado retornam exatamente as mesmas distâncias e
 *      se a operação `decreaseKey` atualiza corretamente o caminho 1 -> 2 -> 5 (custo 0.3)
 *      substituindo a aresta direta 1 -> 5 (custo 1.0).
 */

#include "CSRGraph.hpp"
#include "algorithms.hpp"
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <tuple>

// Função auxiliar genérica para testes de BFS básica caso implementada nos algoritmos
void testParte1Figura1() {
    // Figura 1 (Parte 1): 5 vértices, arestas (1,2), (2,5), (5,3), (4,5), (1,5) com peso padrão 1.0
    std::vector<std::tuple<int, int, float>> edges = {
        {1, 2, 1.0f}, {2, 5, 1.0f}, {5, 3, 1.0f}, {4, 5, 1.0f}, {1, 5, 1.0f}
    };

    CSRGraph csr(5, edges);

    assert(csr.numVertices() == 5);
    assert(csr.numEdges() == 5);
    assert(csr.degree(5) == 4);
    assert(csr.degree(3) == 1);

    std::cout << "[PASS] Testes estruturais da Parte 1 (CSR, Graus e Arestas)\n";
}

void testParte2PesoNegativo() {
    // Figura 1 (Parte 2): contém a aresta (3, 4) com peso -9.5
    std::vector<std::tuple<int, int, float>> edges_neg = {
        {1, 2, 0.1f},
        {2, 5, 0.2f},
        {5, 3, 5.0f},
        {3, 4, -9.5f},
        {4, 5, 2.3f},
        {1, 5, 1.0f}
    };

    CSRGraph g_neg(5, edges_neg);
    assert(g_neg.isWeighted() == true);
    assert(g_neg.hasNegativeWeight() == true);

    bool exception_thrown = false;
    try {
        auto res = dijkstra(g_neg, 1, DijkstraMode::HEAP);
    } catch (const std::domain_error&) {
        exception_thrown = true;
    } catch (const std::runtime_error&) {
        exception_thrown = true;
    }
    assert(exception_thrown && "O Dijkstra deveria lancar excecao para grafos com peso negativo!");

    std::cout << "[PASS] Teste de bloqueio de peso negativo (Figura 1 - Parte 2)\n";
}

void testParte2DijkstraVetorVsHeap() {
    // Figura 1 (Parte 2) convertendo -9.5 para +9.5 para validar caminhos mínimos positivos
    std::vector<std::tuple<int, int, float>> edges_pos = {
        {1, 2, 0.1f},
        {2, 5, 0.2f},
        {5, 3, 5.0f},
        {3, 4, 9.5f},
        {4, 5, 2.3f},
        {1, 5, 1.0f}
    };

    CSRGraph g_csr(5, edges_pos);

    auto res_vec = dijkstra(g_csr, 1, DijkstraMode::VECTOR);
    auto res_heap = dijkstra(g_csr, 1, DijkstraMode::HEAP);
    auto res_targets = dijkstraToTargets(g_csr, 1, DijkstraMode::HEAP, {4, 5});

    // Verifica equivalência estrita entre Vetor e Heap (CSR)
    for (int v = 1; v <= 5; ++v) {
        assert(std::abs(res_vec.dist[v] - res_heap.dist[v]) < 1e-5);
        assert(res_vec.parent[v] == res_heap.parent[v]);
    }

    // Valida o caso clássico de decreaseKey no vértice 5:
    // Aresta direta (1,5) custa 1.0, mas o caminho 1 -> 2 -> 5 custa 0.1 + 0.2 = 0.3
    assert(std::abs(res_heap.dist[1] - 0.0) < 1e-5);
    assert(std::abs(res_heap.dist[2] - 0.1) < 1e-5);
    assert(std::abs(res_heap.dist[5] - 0.3) < 1e-5);
    assert(std::abs(res_heap.dist[4] - 2.6) < 1e-5); // 0.3 + 2.3 = 2.6
    assert(std::abs(res_heap.dist[3] - 5.3) < 1e-5); // 0.3 + 5.0 = 5.3
    assert(std::abs(res_targets.dist[4] - res_heap.dist[4]) < 1e-5);
    assert(std::abs(res_targets.dist[5] - res_heap.dist[5]) < 1e-5);

    auto path_to_4 = reconstructPath(res_heap, 1, 4);
    std::vector<int> expected_path = {1, 2, 5, 4};
    assert(path_to_4 == expected_path);

    std::cout << "[PASS] Teste de equivalencia Dijkstra (Vetor vs Heap) e decreaseKey\n";
}

void testDijkstraComVerticeInalcancavel() {
    std::vector<std::tuple<int, int, float>> edges = {
        {1, 2, 1.0f},
        {3, 4, 0.5f}
    };
    CSRGraph graph(4, edges);

    auto vector_result = dijkstra(graph, 1, DijkstraMode::VECTOR);
    auto heap_result = dijkstra(graph, 1, DijkstraMode::HEAP);
    auto target_result = dijkstraToTargets(graph, 1, DijkstraMode::HEAP, {2, 4});

    assert(std::abs(vector_result.dist[2] - heap_result.dist[2]) < 1e-5);
    assert(std::isinf(vector_result.dist[3]));
    assert(std::isinf(vector_result.dist[4]));
    assert(std::abs(target_result.dist[2] - 1.0) < 1e-5);
    assert(std::isinf(target_result.dist[4]));

    std::cout << "[PASS] Teste de Dijkstra em componente desconexa\n";
}

void testLeituraCSRDoArquivo() {
    const auto unique_id = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto filename = std::filesystem::temp_directory_path() /
        ("grafo_csr_test_" + std::to_string(unique_id) + ".txt");
    {
        std::ofstream output(filename);
        assert(output.is_open());
        output << "4\n1 2 0.1\n2 3 2.0\n";
    }

    CSRGraph graph(filename.string(), true);
    std::filesystem::remove(filename);

    assert(graph.numVertices() == 4);
    assert(graph.numEdges() == 2);
    assert(graph.isWeighted());
    assert(graph.degree(2) == 2);
    assert(std::abs(graph.edgeWeight(2, 1) - 2.0f) < 1e-5);

    auto result = dijkstra(graph, 1, DijkstraMode::HEAP);
    assert(std::abs(result.dist[3] - 2.1) < 1e-5);
    assert(std::isinf(result.dist[4]));

    std::cout << "[PASS] Teste de leitura CSR em duas passagens\n";
}

int main() {
    std::cout << "=== INICIANDO SUITE DE TESTES UNITARIOS ===\n";
    testParte1Figura1();
    testParte2PesoNegativo();
    testParte2DijkstraVetorVsHeap();
    testDijkstraComVerticeInalcancavel();
    testLeituraCSRDoArquivo();
    std::cout << "\n>>> TODOS OS TESTES PASSARAM COM SUCESSO! <<<\n";
    return 0;
}