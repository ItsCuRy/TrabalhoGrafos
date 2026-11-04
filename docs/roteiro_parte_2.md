# Roteiro de Apresentação — Trabalho de Grafos (Parte 2)
**Autores:** Arthur Sampaio e Carlos Bruno  
**Disciplina:** COS 242 — Teoria dos Grafos (PESC/COPPE/UFRJ)  
**Tempo total estimado:** 7 a 8 minutos (9 slides)

---

## Slide 1 — Capa (0:00 – 0:20)
- Olá, professor. Nesta apresentação, eu (Arthur Sampaio) e o Carlos Bruno vamos mostrar as decisões de projeto, otimizações de memória e resultados experimentais da Parte 2 da nossa biblioteca de grafos em C++17, focada em grafos com pesos, caminhos mínimos com o algoritmo de Dijkstra e análise da rede de colaboração científica.

---

## Slide 2 — Escopo e Evolução da Arquitetura (0:20 – 1:10)
- Na Parte 1, comprovamos que a matriz de adjacências (mesmo com `BitMatrix`) é inviável em memória para milhões de vértices por exigir $\Theta(V^2)$. Por isso, na Parte 2 removemos os módulos legados de matriz e consolidamos toda a biblioteca sobre a estrutura **CSR (`CSRGraph`)**.
- Para suportar pesos nas arestas mantendo localidade de cache, adotamos o formato *Structure of Arrays* com três vetores contíguos:
  1. `offsets_` com $n+2$ posições indicando onde começa e termina a vizinhança de cada vértice;
  2. `edges_` com $2m$ inteiros para os vizinhos;
  3. `weights_` com $2m$ valores `float` alinhados posição a posição com `edges_`.
- Com isso, cada aresta não-direcionada consome exatos 16 bytes (8 bytes por direção: 4 do vértice e 4 do peso).

---

## Slide 3 — Leitura CSR em Duas Passagens (1:10 – 2:05)
- Uma das principais otimizações que fizemos nesta etapa foi reescrever o construtor do CSR para ler o arquivo em **duas passagens**, eliminando o vetor temporário `temp_edges` que existia antes.
- Na versão anterior, guardar todas as arestas num vetor auxiliar enquanto o CSR final era alocado quase dobrava o pico de memória RAM.
- Agora:
  - Na **primeira passagem**, apenas lemos o arquivo para contar o grau de cada vértice, validar arestas e já detectar se existe algum peso negativo, sem guardar arestas na memória.
  - Em seguida, calculamos os prefixos em `offsets_` e alocamos `edges_` e `weights_` no tamanho final exato.
  - Na **segunda passagem**, voltamos ao início do arquivo e preenchemos diretamente os vetores finais.
- Isso manteve o consumo de memória estritamente em $\mathcal{O}(V+E)$: o `grafo_W_3` usa apenas 93 MB, o `W_4` usa 476 MB e o `W_5` (com 10 milhões de vértices e 40 milhões de arestas) ocupa apenas 648 MB.

---

## Slide 4 — Algoritmos de Caminho Mínimo (2:05 – 2:55)
- Para calcular distâncias e caminhos mínimos, a biblioteca verifica primeiro a natureza do grafo:
  - Se não tiver pesos, usa BFS em $\mathcal{O}(V+E)$.
  - Se tiver pesos, verifica em $\mathcal{O}(1)$ a flag `hasNegativeWeight()`. Caso haja peso negativo — como na aresta $-9,5$ da Figura 1 do enunciado — a biblioteca bloqueia a execução lançando uma exceção, já que o Dijkstra exige pesos não-negativos.
- Embora os pesos das arestas fiquem em `float` no CSR para economizar memória, o vetor de distâncias acumuladas usa `double` para evitar perda de precisão em caminhos longos.
- Além disso, criamos a função `dijkstraToTargets` com **parada antecipada** exclusivamente para a pergunta 1 (que pede apenas os caminhos do vértice 10 até os vértices 20, 30, 40, 50 e 60). Assim que esses cinco destinos são extraídos como mínimo, suas distâncias já são definitivas e a busca para. Já na pergunta 2 (benchmark), rodamos a busca completa para todos os vértices do grafo.

---

## Slide 5 — Duas Implementações de Dijkstra: Vetor vs. Heap (2:55 – 4:00)
- Implementamos o algoritmo de Dijkstra das duas formas pedidas no enunciado:
  1. **Modo Vetor (`VectorDistanceStore`):** guarda as estimativas em um vetor e, a cada passo, faz uma busca linear $\mathcal{O}(V)$ para encontrar o vértice não visitado de menor distância. Também adicionamos um controle com o vetor `queued_` e um contador de candidatos ativos para interromper imediatamente caso a componente conexa acabe antes de visitar todos os vértices. Mesmo assim, a complexidade dominante é quadrática: $\mathcal{O}(V^2 + E)$.
  2. **Modo Heap (`IndexedMinHeap`):** em vez de usar `std::priority_queue` do C++ (que não possui `decreaseKey` e insere vértices duplicados até $\mathcal{O}(E)$), implementamos do zero um **Heap Binário Mínimo Indexado**. Ele mantém um vetor `pos_` com a posição exata de cada vértice dentro do heap em $\mathcal{O}(1)$. Quando uma distância melhora no relaxamento, fazemos `decreaseKey` via `siftUp` em $\mathcal{O}(\log V)$, garantindo no máximo $V$ elementos no heap e complexidade total $\mathcal{O}((V+E)\log V)$.

---

## Slide 6 — Estudo de Caso 1: Distâncias e Caminhos Mínimos (4:00 – 4:55)
- Neste slide temos os resultados do Item 1 para os grafos `W_3` ($10^5$ vértices), `W_4` ($10^6$ vértices) e `W_5` ($10^7$ vértices), saindo do vértice 10 para 20, 30, 40, 50 e 60.
- Dois pontos interessantes aparecem nos resultados:
  - Primeiro, a formação clara da **árvore de caminhos mínimos**: no `W_3`, os caminhos para 20, 40 e 60 compartilham o mesmo tronco inicial de 5 vértices (`10 -> 2858 -> 35293 -> 3905 -> 24445`); no `W_4`, os destinos de 30 a 60 também saem pelo mesmo prefixo.
  - Segundo, fica evidente a diferença entre distância em saltos (BFS) e distância ponderada (Dijkstra): no `W_3`, o caminho mais curto em peso é para o vértice 20 (distância $0,80$), mas ele usa **18 arestas** de peso muito pequeno; já para o vértice 50 são apenas **9 arestas**, porém com soma maior ($0,87$).

---

## Slide 7 — Estudo de Caso 1: Desempenho Heap vs. Vetor (4:55 – 6:15)
- Na tabela do Item 2 comparamos o tempo de busca completa do Heap contra o Vetor, sorteando exatamente as mesmas origens com semente fixa.
- Nos grafos a partir de $100.000$ vértices, cada busca completa do vetor já faz na ordem de $10^{10}$ a $10^{14}$ operações, então ajustamos para $k=1$ busca completa para tornar a medição viável.
- Os números confirmam a teoria de forma exata:
  - Quando aumentamos $V$ em **10 vezes** do `W_3` ($10^5$) para o `W_4` ($10^6$), a complexidade $\mathcal{O}(V^2)$ do vetor prevê um aumento de $10^2 = 100$ vezes no tempo. Na prática, o tempo do vetor subiu de **16,89 segundos** para **1.681,75 segundos (28 minutos)** — um fator medido de **99,54 vezes**.
  - Seguindo essa mesma progressão quadrática, uma única busca vetorial no `W_5` e no `W_6` ($10^7$ vértices, $10\times$ maior que o `W_4`) leva cerca de **46,7 horas** ($\approx 2$ dias cada), e no `W_7` ($50$ milhões de vértices) levaria cerca de **49 dias**, passando de **52 dias de CPU** no total.
  - Já o nosso **Heap Indexado** rodou o `W_3` em **0,13 s**, o `W_4` em **1,68 s** (quase **1.000 vezes mais rápido** que o vetor) e resolveu a busca completa para os **10 milhões de vértices do `W_5` em apenas 24,86 segundos**.

---

## Slide 8 — Estudo de Caso 2: Rede de Colaboração DBLP (6:15 – 7:10)
- No segundo estudo de caso, carregamos a rede de coautoria da DBLP, com **722 mil pesquisadores** e **2,27 milhões de arestas**, onde o peso é o inverso do número de artigos em coautoria.
- Calculamos os caminhos mínimos a partir de **Edsger W. Dijkstra** (`ID 2722`) para os cinco pesquisadores solicitados:
  - Para **Alan M. Turing**, a distância é **infinita**, pois Turing faleceu em 1954 e suas publicações não possuem cadeia de coautoria conectada à componente conexa gigante moderna.
  - Para **J. B. Kruskal**, distância $3,4804$ em 8 coautorias.
  - Para **Jon M. Kleinberg**, a menor distância entre todos: $2,7070$ em 9 coautorias.
  - Para **Éva Tardos**, distância $2,7535$ em 11 coautorias (passando por Tarjan e Goldberg).
  - E para o professor **Daniel R. Figueiredo**, distância $2,9428$ em 8 coautorias, chegando através de Zhi-Li Zhang e Donald F. Towsley.

---

## Slide 9 — Interpretação e Conclusões (7:10 – 7:50)
- Para concluir, vale notar na rede de colaboração que todos os caminhos a partir de Dijkstra saem por duas "pontes" principais:
  1. O ramo holandês com sua ex-doutoranda *van Gasteren*, *Gerard Tel* e *Bodlaender*, que conecta Dijkstra à área de Algoritmos e chega em **Kleinberg** e **Éva Tardos**;
  2. E o ramo de sistemas com *John R. Rice* e *Dan Marinescu*, que chega em **Kruskal** e no professor **Daniel Figueiredo**.
- Em resumo, a leitura CSR em duas passagens permitiu trabalhar com grafos de dezenas de milhões de arestas usando pouca RAM, e o Heap Binário Indexado provou na prática por que estruturas $\mathcal{O}((V+E)\log V)$ são essenciais quando saímos da escala de milhares para dezenas de milhões de vértices.
- Muito obrigado! Estamos à disposição para perguntas.