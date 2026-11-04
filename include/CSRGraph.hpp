#pragma once
#include "Graph.hpp"
#include <vector>
#include <string>
#include <utility>
#include <tuple>

/**
 * @brief Implementação de Grafo utilizando Compressed Sparse Row (CSR).
 * 
 * Decisão de Projeto:
 * Utiliza o padrão Structure of Arrays (SoA) mantendo vetores separados e contíguos
 * para offsets (offsets_), destinos das arestas (edges_) e pesos (weights_).
 * Isso maximiza a localidade espacial de cache L1/L2 durante a travessia no Dijkstra
 * e evita o overhead de ponteiros de listas encadeadas ou std::vector<std::vector<...>>.
 */
class CSRGraph : public Graph {
private:
    int n_ = 0;                      ///< Número de vértices
    long long m_ = 0;                ///< Número de arestas não-direcionadas
    bool weighted_ = true;           ///< Flag indicando se o grafo é ponderado
    bool has_negative_ = false;      ///< Flag indicando presença de peso negativo

    /**
     * Vetor de índices (tamanho n_ + 2 para indexação 1-based).
     * Os vizinhos do vértice u estão no intervalo [offsets_[u], offsets_[u + 1]).
     */
    std::vector<int> offsets_;

    /// Vetor contíguo contendo os vértices de destino (tamanho 2 * m_).
    std::vector<int> edges_;

    /// Vetor contíguo paralelo a edges_ contendo os pesos das arestas (tamanho 2 * m_).
    std::vector<float> weights_;

public:
    /**
     * @brief Constrói o grafo em formato CSR diretamente a partir de um arquivo texto.
     * @param filename Caminho para o arquivo (formato: linha 1 = N; demais linhas = u v [w]).
     * @param weighted Se true, lê a terceira coluna como peso (float).
     */
    explicit CSRGraph(const std::string& filename, bool weighted = true);

    /**
     * @brief Construtor em memória para grafos não-ponderados (útil para testes).
     */
    CSRGraph(int n, const std::vector<std::pair<int, int>>& edges);

    /**
     * @brief Construtor em memória para grafos ponderados (útil para testes unitários).
     */
    CSRGraph(int n, const std::vector<std::tuple<int, int, float>>& weighted_edges);

    // Implementações da interface Graph
    int numVertices() const override;
    long long numEdges() const override;
    bool isWeighted() const override;
    bool hasNegativeWeight() const override;
    int degree(int u) const override;
    const int* neighborsBegin(int u) const override;
    const int* neighborsEnd(int u) const override;
    float edgeWeight(int u, int k) const override;
    std::size_t memoryUsageBytes() const override;

    /// Acesso direto ao ponteiro de pesos do vértice u para iteração rápida no CSR
    const float* weightsBegin(int u) const {
        return weights_.empty() ? nullptr : &weights_[offsets_[u]];
    }
};