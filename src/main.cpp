#include "CSRGraph.hpp"
#include "algorithms.hpp"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <random>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @brief Resolve o caminho de um arquivo na pasta data/, funcionando tanto
 * quando executado da raiz do projeto quanto de dentro do diretório build/.
 */
std::string resolveDataPath(const std::string& filename) {
    const std::vector<std::string> candidates = {
        "data/" + filename,
        "../data/" + filename,
        filename
    };
    for (const auto& candidate : candidates) {
        if (fs::exists(candidate)) {
            return candidate;
        }
    }
    return "data/" + filename;
}

/**
 * @brief Formata um vetor de vértices como string legível: "10 -> 25 -> 20".
 */
std::string formatPath(const std::vector<int>& path) {
    if (path.empty()) return "Inalcancavel";
    std::ostringstream oss;
    for (std::size_t i = 0; i < path.size(); ++i) {
        oss << path[i];
        if (i + 1 < path.size()) oss << " -> ";
    }
    return oss.str();
}

/**
 * @brief Executa os Estudos de Caso 1 e 2 para um grafo ponderado grafo_W_X.txt:
 *  - Item 1: Distância e caminho mínimo entre o vértice 10 e os vértices {20, 30, 40, 50, 60}.
 *  - Item 2: Tempo médio de execução do algoritmo de Dijkstra para k vértices aleatórios
 *            comparando as implementações com Vetor e com Heap Binário Indexado.
 */
void runStandardCaseStudy(const std::string& filename, int k_samples = 100) {
    std::string filepath = resolveDataPath(filename);

    std::cout << "\n====================================================================\n";
    std::cout << "  ESTUDO DE CASO: " << filename << "\n";
    std::cout << "====================================================================\n";

    if (!fs::exists(filepath)) {
        std::cout << "[AVISO] Arquivo nao encontrado em '" << filepath << "'. Pulando...\n";
        return;
    }

    try {
        std::cout << "[1/4] Lendo grafo de entrada..." << std::endl;
        auto t0 = std::chrono::high_resolution_clock::now();
        CSRGraph graph(filepath, true);
        auto t1 = std::chrono::high_resolution_clock::now();
        double load_time = std::chrono::duration<double>(t1 - t0).count();

        std::cout << "Vertices (V): " << graph.numVertices()
                  << " | Arestas (E): " << graph.numEdges()
                  << " | Memoria CSR: " << std::fixed << std::setprecision(2)
                  << (graph.memoryUsageBytes() / (1024.0 * 1024.0)) << " MB"
                  << " | Leitura: " << load_time << " s\n\n";

        // -------------------------------------------------------------------------
        // Pergunta 1: Distância e caminho mínimo de 10 para {20, 30, 40, 50, 60}
        // -------------------------------------------------------------------------
        const int source = 10;
        const std::vector<int> targets = {20, 30, 40, 50, 60};

        std::cout << "[2/4] Calculando caminhos de " << source
              << " para os cinco destinos..." << std::endl;
        ShortestPathResult sp = dijkstraToTargets(graph, source, DijkstraMode::HEAP, targets);

        std::cout << "--- [Item 1] Distancias e Caminhos Minimos (Origem = " << source << ") ---\n";
        std::cout << std::left << std::setw(10) << "Destino"
                  << std::setw(18) << "Distancia"
                  << "Caminho Minimo\n";
        std::cout << std::string(70, '-') << "\n";

        for (int t : targets) {
            std::vector<int> path = reconstructPath(sp, source, t);
            std::cout << std::left << std::setw(10) << t;
            if (path.empty()) {
                std::cout << std::setw(18) << "Inf" << "Inalcancavel\n";
            } else {
                std::cout << std::setw(18) << std::fixed << std::setprecision(4) << sp.dist[t]
                          << formatPath(path) << "\n";
            }
        }

        // -------------------------------------------------------------------------
        // Pergunta 2: Tempo médio para k vértices iniciais escolhidos aleatoriamente
        // -------------------------------------------------------------------------
        std::cout << "\n--- [Item 2] Tempo Medio de Execucao do Dijkstra ---\n";

        std::mt19937 rng(42); // Semente fixa para reprodutibilidade científica
        std::uniform_int_distribution<int> dist_vertex(1, graph.numVertices());

        // O limite controla apenas quantas buscas completas são repetidas, nunca os vértices visitados.
        constexpr long long max_vector_scan_work = 100'000'000;
        const long long vertex_count = graph.numVertices();
        const long long work_per_vector_search = vertex_count * vertex_count;
        int sample_count = k_samples;
        if (work_per_vector_search > max_vector_scan_work) {
            sample_count = static_cast<int>(max_vector_scan_work / work_per_vector_search);
            if (sample_count < 1) sample_count = 1;
        }

        std::vector<int> random_sources(sample_count);
        for (int i = 0; i < sample_count; ++i) {
            random_sources[i] = dist_vertex(rng);
        }

        // 2.1 Avaliação com HEAP BINÁRIO INDEXADO
        std::cout << "[3/4] Medindo Dijkstra com heap (k = " << sample_count
                  << ", buscas completas)..." << std::endl;
        auto th_start = std::chrono::high_resolution_clock::now();
        for (int s : random_sources) {
            ShortestPathResult r = dijkstra(graph, s, DijkstraMode::HEAP);
            (void)r;
        }
        auto th_end = std::chrono::high_resolution_clock::now();
        double heap_total_s = std::chrono::duration<double>(th_end - th_start).count();
        double heap_avg_ms = (heap_total_s * 1000.0) / sample_count;

        std::cout << "  * Dijkstra [HEAP]   (k = " << sample_count << "): "
                  << std::fixed << std::setprecision(4) << heap_avg_ms << " ms/busca "
                  << "(Tempo total: " << heap_total_s << " s)\n";

        // 2.2 Avaliação com VETOR
        // Usa as mesmas origens do heap; cada execução ainda calcula distâncias no grafo inteiro.
        std::cout << "[4/4] Medindo Dijkstra com vetor (k = " << sample_count
                  << ", buscas completas)..." << std::endl;
        auto tv_start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < sample_count; ++i) {
            ShortestPathResult r = dijkstra(graph, random_sources[i], DijkstraMode::VECTOR);
            (void)r;
        }
        auto tv_end = std::chrono::high_resolution_clock::now();
        double vec_total_s = std::chrono::duration<double>(tv_end - tv_start).count();
        double vec_avg_ms = (vec_total_s * 1000.0) / sample_count;

        std::cout << "  * Dijkstra [VECTOR] (k = " << sample_count << "): "
                  << std::fixed << std::setprecision(4) << vec_avg_ms << " ms/busca "
                  << "(Tempo total: " << vec_total_s << " s)\n";

    } catch (const std::exception& e) {
        std::cout << "[ERRO ao processar " << filename << "]: " << e.what() << "\n";
    }
}

/**
 * @brief Executa o Estudo de Caso da Rede de Colaboração entre pesquisadores da Computação.
 * 
 * Mapeia os nomes em `rede_colaboracao_vertices.txt` para seus respectivos IDs numéricos
 * e calcula a distância e o caminho mínimo de "Edsger W. Dijkstra" para:
 *  - Alan M. Turing
 *  - J. B. Kruskal
 *  - Jon M. Kleinberg
 *  - Éva Tardos
 *  - Daniel R. Figueiredo
 */
void runCollaborationStudy() {
    std::string edges_path = resolveDataPath("rede_colaboracao.txt");
    std::string vertices_path = resolveDataPath("rede_colaboracao_vertices.txt");

    std::cout << "\n====================================================================\n";
    std::cout << "  ESTUDO DE CASO: REDE DE COLABORACAO CIENTIFICA (DBLP)\n";
    std::cout << "====================================================================\n";

    if (!fs::exists(edges_path) || !fs::exists(vertices_path)) {
        std::cout << "[AVISO] Arquivos da rede de colaboracao nao encontrados na pasta data/.\n";
        return;
    }

    // Leitura do dicionário de pesquisadores (formato: ID,Nome do Pesquisador)
    std::unordered_map<std::string, int> name_to_id;
    std::vector<std::string> id_to_name;

    std::ifstream vfile(vertices_path);
    std::string line;
    while (std::getline(vfile, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        std::size_t comma_pos = line.find(',');
        if (comma_pos == std::string::npos) continue;

        int id = std::stoi(line.substr(0, comma_pos));
        std::string name = line.substr(comma_pos + 1);

        if (id >= static_cast<int>(id_to_name.size())) {
            id_to_name.resize(id + 1);
        }
        id_to_name[id] = name;
        name_to_id[name] = id;
    }

    CSRGraph graph(edges_path, true);
    std::cout << "Rede carregada -> Pesquisadores (V): " << graph.numVertices()
              << " | Coautorias (E): " << graph.numEdges() << "\n\n";

    const std::string source_researcher = "Edsger W. Dijkstra";
    const std::vector<std::string> target_researchers = {
        "Alan M. Turing",
        "J. B. Kruskal",
        "Jon M. Kleinberg",
        "Éva Tardos",
        "Daniel R. Figueiredo"
    };

    auto it_src = name_to_id.find(source_researcher);
    if (it_src == name_to_id.end()) {
        std::cerr << "[ERRO] Pesquisador origem nao encontrado: " << source_researcher << "\n";
        return;
    }

    const int src_id = it_src->second;
    ShortestPathResult sp = dijkstra(graph, src_id, DijkstraMode::HEAP);

    for (const auto& target_name : target_researchers) {
        std::cout << "--------------------------------------------------------------------\n";
        std::cout << "Pesquisador Alvo: " << target_name << "\n";

        auto it_tgt = name_to_id.find(target_name);
        if (it_tgt == name_to_id.end()) {
            std::cout << "  [!] Nome nao encontrado no arquivo de vertices.\n";
            continue;
        }

        int tgt_id = it_tgt->second;
        std::vector<int> path = reconstructPath(sp, src_id, tgt_id);

        if (path.empty()) {
            std::cout << "  Distancia: Inf (Sem caminho de coautoria na componente conexa)\n";
        } else {
            std::cout << "  ID do Vertice:    " << tgt_id << "\n";
            std::cout << "  Distancia Minima: " << std::fixed << std::setprecision(6) << sp.dist[tgt_id] << "\n";
            std::cout << "  Caminho Minimo (" << (path.size() - 1) << " coautoria(s)):\n    ";
            for (std::size_t i = 0; i < path.size(); ++i) {
                int u = path[i];
                std::cout << id_to_name[u] << " [" << u << "]";
                if (i + 1 < path.size()) std::cout << " -> ";
            }
            std::cout << "\n";
        }
    }
    std::cout << "--------------------------------------------------------------------\n";
}

int main(int argc, char* argv[]) {
    // Se o usuário passar um arquivo específico por linha de comando, roda apenas ele
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "colaboracao") {
            runCollaborationStudy();
        } else {
            runStandardCaseStudy(arg, 100);
        }
        return 0;
    }

    // Execução padrão: processa grafo_W_3.txt até grafo_W_7.txt e a rede de colaboração
    for (int i = 3; i <= 7; ++i) {
        runStandardCaseStudy("grafo_W_" + std::to_string(i) + ".txt", 100);
    }

    runCollaborationStudy();

    return 0;
}