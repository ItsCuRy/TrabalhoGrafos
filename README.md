# Trabalho de Grafos — COS 242 (Parte 2)

Aplicação C++17 para grafos não direcionados ponderados, com representação CSR e
algoritmo de Dijkstra usando vetor ou heap binário indexado.

## Build e testes

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Os testes usam `assert`; o alvo `test_grafo` mantém essas verificações ativas
mesmo em builds Release.

Para criar uma build limpa, use outro diretório:

```bash
cmake -S . -B build-clean -DCMAKE_BUILD_TYPE=Release
cmake --build build-clean --parallel
ctest --test-dir build-clean --output-on-failure
```

## Executar

```bash
./build/grafo data/grafo_W_3.txt
./build/grafo colaboracao
```

Sem argumentos, o programa processa os arquivos `grafo_W_3.txt` a
`grafo_W_7.txt` e a rede de colaboração, quando disponíveis em `data/`.

## Organização

- `include/` e `src/`: interface `Graph`, implementação CSR e algoritmos.
- `tests/test_grafo.cpp`: testes em memória dos grafos e do Dijkstra.
- `scripts/run_estudos.sh`: compilação, testes e execução dos estudos de caso.
- `docs/`: relatório e apresentação do trabalho.
