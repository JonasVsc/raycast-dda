# raycast-dda

## Algoritmo:

  Raycasting com DDA (Digital Differential Analyzer) - Cálculo de interseção de raios em grades e renderização pseudo-3D

## Resumo:

O Raycasting com DDA é um algoritmo que projeta raios a partir de uma origem em um mapa de grid 2D para encontrar o objeto mais próximo que bloqueia o caminho desse raio. Utiliza passos incrementais (DDA) para calcular a intersecção com obstáculos, determinando a distância exata até o ponto de impacto. 
 
Complexidade pior caso: O(W * D), onde W é a resolução de raios e D a profundidade, ocorre quando não há nenhuma colisão, cada raio atinge o limite máximo de profundidade.

Complexidade melhor caso: O(W), ocorre quando a colisão é encontrada no primeiro passo.

Dados de entrada: Matriz 2D do mapa e vetores de posição/direção da origem.

Dados de saída: Array float contendo as distâncias perpendiculares exatas de cada raio até o ponto de colisão.