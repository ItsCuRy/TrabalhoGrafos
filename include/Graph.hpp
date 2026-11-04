#pragma once
#include <cstddef>

/**
 * @brief Interface abstrata para representação de grafos (ponderados ou não).
 * 
 * Adota indexação 1-based (vértices de 1 até n) para manter compatibilidade
 * direta com os arquivos de entrada da disciplina COS242.
 */
class Graph {
public:
    virtual ~Graph() = default;

    /// Retorna o número total de vértices (n).
    virtual int numVertices() const = 0;

    /// Retorna o número total de arestas (m).
    virtual long long numEdges() const = 0;

    /// Indica se o grafo possui pesos associados às arestas.
    virtual bool isWeighted() const = 0;

    /// Indica se o grafo contém alguma aresta com peso negativo (w < 0).
    virtual bool hasNegativeWeight() const = 0;

    /// Retorna o grau do vértice u (1 <= u <= n).
    virtual int degree(int u) const = 0;

    /// Ponteiro para o início da lista contígua de vizinhos do vértice u.
    virtual const int* neighborsBegin(int u) const = 0;

    /// Ponteiro para o fim (one-past-the-end) da lista de vizinhos do vértice u.
    virtual const int* neighborsEnd(int u) const = 0;

    /// Retorna o peso da k-ésima aresta incidente ao vértice u (0 <= k < degree(u)).
    virtual float edgeWeight(int u, int k) const = 0;

    /// Retorna a estimativa de memória consumida pela estrutura em bytes.
    virtual std::size_t memoryUsageBytes() const = 0;
};