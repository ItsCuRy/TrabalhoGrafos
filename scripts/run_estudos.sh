#!/usr/bin/env bash
# ==============================================================================
# Script de Automação dos Estudos de Caso — Teoria dos Grafos (COS 242 - Parte 2)
# ==============================================================================
# Como usar:
#   chmod +x scripts/run_estudos.sh
#   ./scripts/run_estudos.sh
# ==============================================================================

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
DATA_DIR="${ROOT_DIR}/data"
RESULTS_FILE="${ROOT_DIR}/resultados_parte2.txt"

echo "[1/3] Compilando a biblioteca e executaveis em modo Release (-O3)..."
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j"$(nproc)"

echo "[2/3] Executando suite de testes unitarios..."
if [[ -f "./test_grafo" ]]; then
    ./test_grafo
else
    echo "[AVISO] Executavel test_grafo nao encontrado. Pulando testes..."
fi

echo "[3/3] Iniciando Estudos de Caso da Parte 2..."
echo "Resultados gerados em $(date)" > "${RESULTS_FILE}"

# Executavel principal gerado pelo CMake.
GRAFO_BIN="${BUILD_DIR}/grafo"

if [[ ! -f "${GRAFO_BIN}" ]]; then
    echo "[ERRO] Executavel 'grafo' nao encontrado em ${BUILD_DIR}. Verifique a compilacao."
    exit 1
fi

# Executa o estudo da Rede de Colaboração caso os arquivos estejam em data/
if [[ -f "${DATA_DIR}/rede_colaboracao.txt" && -f "${DATA_DIR}/rede_colaboracao_vertices.txt" ]]; then
    echo "Processando Rede de Colaboracao..."
    echo "--- REDE DE COLABORACAO ---" >> "${RESULTS_FILE}"

    "${GRAFO_BIN}" colaboracao 2>&1 | tee -a "${RESULTS_FILE}"
fi

# Executa para cada grafo ponderado encontrado em data/grafo_W_*.txt
for gfile in "${DATA_DIR}"/grafo_W_*.txt; do
    if [[ -f "${gfile}" ]]; then
        echo "Processando ${gfile}..."
        echo "--- GRAFO: $(basename "${gfile}") ---" >> "${RESULTS_FILE}"

        "${GRAFO_BIN}" "${gfile}" 2>&1 | tee -a "${RESULTS_FILE}"

        echo "" >> "${RESULTS_FILE}"
    fi
done

echo "Concluido! Todos os resultados foram salvos em: ${RESULTS_FILE}"