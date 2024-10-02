Razões da mudança do BP para o SBBU

O espaço de busca do DMDGP pode ser representado por variáveis binárias, cada uma delas associada à posição relativa de um vértice com respeito aos seus três antecessores. O BP faz uma enumeração do espaço das variáveis binárias, construindo gradativamente uma solução, fixando uma variável binária por vez. Caso a solução parcial seja inviável, ela é descartada e a próxima solução parcial é testada. Na tentativa de construir uma solução viável, o BP não é guiado pelas restrições de poda, ainda que as utilize para descartar soluções inviáveis.

O SBBU por outro lado, resolve uma restrição por vez seguindo uma ordenação prévia das arestas de poda. Esta estratégia é inspirada no fato que, com probabilidade 1, cada restrição $\{i,j\}$ possui apenas duas soluções binárias $b_{[i,j]} = b_i, \ldots, b_j$, sendo uma o flip da outra \cite{Liberti}. Como consequência, diferentemente do BP, o SBBU utiliza cada restrição de poda apenas uma vez e combina as soluções $b_{[i,j]}$ para a construção de soluções completas $b_{[1,n]}$.

A satisfação da restrição $\{i,j\}$ depende da posição relativa de $x_i$ e $x_j$. As variáveis binárias que determinam a posição relativa de $x_i$ e $x_j$ são $b_{i+3},\ldots, b_j$. Diremos que a restrição $\{i,j\}$ é dependente das variáveis binárias $b_{i+3},\ldots, b_j$. 

Diremos que duas arestas de poda $e$ e $e^\prime$ estão acopladas se elas dependem de, pelo menos, uma mesma variável binária.

Dada uma ordenação $e_1,\ldots, e_m$ das areastas de poda,diremos que uma aresta de poda $e_k$ é independente se ela não está acoplada a arestas anteriores.

Quando buscamos uma "solução" para uma aresta independente, não há padrões binários que possam ser utilizados para reduzir o número de variáveis. Sendo assim, teremos de buscar uma sequência binária viável $b_{[i,j]}$ de forma exaustiva. Neste caso, podemos utilizar uma busca Depth-First Search (DFS), onde as sequências binárias são testadas em ordem lexicográfica ou, alternativamente, podemos utilizar uma ordenação baseada em frequências, priorizando as sequências mais prováveis de ocorrerem. Esta alternativa é a que denominamos de Frequency Based Search (FBS).


