#include "algorithms.hpp"
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <utility>

namespace {

constexpr double INF = std::numeric_limits<double>::infinity();

/**
 * @brief Armazenamento de estimativas de distância baseado em Vetor.
 * 
 * Decisão de Projeto:
 * - Inserção / atualização de chave (pushOrDecreaseKey): O(1), apenas atualiza o vetor
 *   e mantém a lista de candidatos ativos.
 * - Extração do mínimo (extractMin): O(V) por varredura linear sobre todos os vértices
 *   ainda não visitados, resultando em complexidade total O(V^2 + E) no Dijkstra.
 */
class VectorDistanceStore {
private:
    int n_;
    const std::vector<double>& dist_;
    std::vector<unsigned char> visited_;
    std::vector<unsigned char> queued_;
    int remaining_active_ = 0;

public:
    VectorDistanceStore(int n, const std::vector<double>& dist)
        : n_(n), dist_(dist), visited_(n + 1, 0), queued_(n + 1, 0), remaining_active_(0) {}

    bool empty() const {
        return remaining_active_ == 0;
    }

    void pushOrDecreaseKey(int u, double /*new_dist*/) {
        // Conta cada candidato uma vez; novas distâncias não criam entradas duplicadas.
        if (!visited_[u] && !queued_[u]) {
            queued_[u] = 1;
            remaining_active_++;
        }
    }

    int extractMin() {
        int best_u = -1;
        double min_d = INF;

        // Busca linear O(V) pelo vértice não visitado de menor distância estimada
        for (int v = 1; v <= n_; ++v) {
            if (!visited_[v] && dist_[v] < min_d) {
                min_d = dist_[v];
                best_u = v;
            }
        }

        if (best_u == -1) return -1;

        visited_[best_u] = 1;
        queued_[best_u] = 0;
        remaining_active_--;
        return best_u;
    }
};

/**
 * @brief Fila de prioridades baseada em Heap Binário Mínimo Indexado (Indexed Min-Heap).
 * 
 * Decisão de Projeto:
 * Diferente de std::priority_queue (que não suporta decrease-key e exige inserções
 * duplicadas no heap), este Heap Indexado mantém um vetor `pos_[v]` que mapeia a posição
 * exata do vértice `v` dentro do array `heap_`.
 * - Garante no máximo V elementos no heap simultaneamente (uso de memória estrito O(V)).
 * - Operação pushOrDecreaseKey executa siftUp diretamente na posição `pos_[v]` em O(log V).
 * - Operação extractMin executa em O(log V).
 * - Complexidade total do Dijkstra: O((V + E) log V).
 */
class HeapDistanceStore {
private:
    std::vector<int> heap_;     ///< Array 0-based representando a árvore binária do heap
    std::vector<int> pos_;      ///< pos_[v] = índice de v em heap_ (ou -1 se fora do heap)
    std::vector<double> keys_;  ///< Chave (distância atual) associada a cada vértice
    int size_ = 0;              ///< Quantidade atual de elementos no heap

    void swapNodes(int i, int j) {
        int u = heap_[i];
        int v = heap_[j];
        std::swap(heap_[i], heap_[j]);
        pos_[u] = j;
        pos_[v] = i;
    }

    void siftUp(int idx) {
        while (idx > 0) {
            int parent = (idx - 1) / 2;
            if (keys_[heap_[idx]] < keys_[heap_[parent]]) {
                swapNodes(idx, parent);
                idx = parent;
            } else {
                break;
            }
        }
    }

    void siftDown(int idx) {
        while (true) {
            int left = 2 * idx + 1;
            int right = 2 * idx + 2;
            int smallest = idx;

            if (left < size_ && keys_[heap_[left]] < keys_[heap_[smallest]]) {
                smallest = left;
            }
            if (right < size_ && keys_[heap_[right]] < keys_[heap_[smallest]]) {
                smallest = right;
            }
            if (smallest != idx) {
                swapNodes(idx, smallest);
                idx = smallest;
            } else {
                break;
            }
        }
    }

public:
    HeapDistanceStore(int n, const std::vector<double>& /*dist*/)
        : heap_(n + 1, 0), pos_(n + 1, -1), keys_(n + 1, INF), size_(0) {}

    bool empty() const {
        return size_ == 0;
    }

    void pushOrDecreaseKey(int u, double new_dist) {
        keys_[u] = new_dist;
        if (pos_[u] == -1) {
            // Vértice ainda não estava no heap: insere na última folha e sobe
            int idx = size_++;
            heap_[idx] = u;
            pos_[u] = idx;
            siftUp(idx);
        } else {
            // Vértice já está no heap: atualiza a chave in-place e restaura a propriedade do heap
            siftUp(pos_[u]);
        }
    }

    int extractMin() {
        if (size_ == 0) return -1;

        int min_v = heap_[0];
        pos_[min_v] = -1;
        size_--;

        if (size_ > 0) {
            heap_[0] = heap_[size_];
            pos_[heap_[0]] = 0;
            siftDown(0);
        }
        return min_v;
    }
};

/**
 * @brief Template interno que unifica a lógica de relaxamento do algoritmo de Dijkstra.
 * 
 * Como o tipo `DistanceStore` é resolvido em tempo de compilação (Zero-Cost Abstraction),
 * o compilador pode aplicar inlining completo nas operações do Heap ou do Vetor sem
 * custo de chamadas virtuais dentro do laço crítico.
 */
template <typename DistanceStore>
ShortestPathResult runDijkstraImpl(const Graph& g, int source,
                                  const std::vector<int>* targets = nullptr) {
    const int n = g.numVertices();

    ShortestPathResult res;
    res.dist.assign(n + 1, INF);
    res.parent.assign(n + 1, 0);

    res.dist[source] = 0.0;

    std::vector<unsigned char> is_target;
    int remaining_targets = 0;
    if (targets != nullptr) {
        is_target.assign(n + 1, 0);
        for (int target : *targets) {
            if (!is_target[target]) {
                is_target[target] = 1;
                remaining_targets++;
            }
        }
        if (remaining_targets == 0) return res;
    }

    DistanceStore store(n, res.dist);
    store.pushOrDecreaseKey(source, 0.0);

    while (!store.empty()) {
        int u = store.extractMin();
        if (u == -1) break;

        // Dijkstra finaliza um vértice ao extraí-lo como o mínimo; destinos restantes
        // não precisam ser explorados quando todos os destinos solicitados já fecharam.
        if (targets != nullptr && is_target[u] && --remaining_targets == 0) break;

        const double d_u = res.dist[u];
        const int* nbr_begin = g.neighborsBegin(u);
        const int deg = g.degree(u);

        // Percorre os vizinhos contíguos na estrutura CSR
        for (int k = 0; k < deg; ++k) {
            int v = nbr_begin[k];
            double w = static_cast<double>(g.edgeWeight(u, k));

            if (d_u + w < res.dist[v]) {
                res.dist[v] = d_u + w;
                res.parent[v] = u;
                store.pushOrDecreaseKey(v, res.dist[v]);
            }
        }
    }

    return res;
}

} // namespace anônimo

namespace {

void validateDijkstraInput(const Graph& g, int source) {
    if (source < 1 || source > g.numVertices()) {
        throw std::out_of_range("Vertice de origem fora do intervalo [1, n].");
    }

    if (g.hasNegativeWeight()) {
        throw std::domain_error(
            "Erro: O grafo possui pesos negativos. A biblioteca ainda nao implementa "
            "caminhos minimos com pesos negativos."
        );
    }
}

} // namespace

ShortestPathResult dijkstra(const Graph& g, int source, DijkstraMode mode) {
    validateDijkstraInput(g, source);

    if (mode == DijkstraMode::HEAP) {
        return runDijkstraImpl<HeapDistanceStore>(g, source);
    } else {
        return runDijkstraImpl<VectorDistanceStore>(g, source);
    }
}

ShortestPathResult dijkstraToTargets(const Graph& g, int source, DijkstraMode mode,
                                    const std::vector<int>& targets) {
    validateDijkstraInput(g, source);
    for (int target : targets) {
        if (target < 1 || target > g.numVertices()) {
            throw std::out_of_range("Vertice de destino fora do intervalo [1, n].");
        }
    }

    if (mode == DijkstraMode::HEAP) {
        return runDijkstraImpl<HeapDistanceStore>(g, source, &targets);
    }
    return runDijkstraImpl<VectorDistanceStore>(g, source, &targets);
}

std::vector<int> reconstructPath(const ShortestPathResult& res, int source, int target) {
    if (target < 1 || target >= static_cast<int>(res.dist.size())) {
        return {};
    }
    if (res.dist[target] == INF) {
        return {};
    }

    std::vector<int> path;
    for (int curr = target; curr != 0; curr = res.parent[curr]) {
        path.push_back(curr);
        if (curr == source) break;
    }

    if (path.empty() || path.back() != source) {
        return {};
    }

    std::reverse(path.begin(), path.end());
    return path;
}