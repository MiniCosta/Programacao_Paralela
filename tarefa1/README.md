# Memória Cache, Localidade Espacial e Temporal, e Row Major vs Column Major em C

## Memória Cache

A memória cache é uma memória rápida localizada entre o processador e a memória principal (RAM). Ela armazena dados frequentemente acessados para acelerar o processamento, reduzindo o tempo de acesso à memória.

## Localidade Espacial e Temporal

- **Localidade Espacial:** Refere-se à tendência de acessar dados próximos uns dos outros na memória em um curto intervalo de tempo.
- **Localidade Temporal:** Refere-se à tendência de acessar repetidamente os mesmos dados em curtos períodos.

Esses conceitos são explorados pela cache para melhorar o desempenho.

## Row Major vs Column Major em C

Em C, arrays multidimensionais são armazenados em **row major order**, ou seja, elementos de uma mesma linha estão em posições consecutivas na memória. Quando acessamos os elementos linha por linha, aproveitamos a localidade espacial, pois os dados já estão próximos na memória, tornando o acesso mais rápido devido ao cache.

Já no **column major order** (usado em outras linguagens como Fortran e MATLAB), os elementos de uma mesma coluna estão próximos. Em C, acessar coluna por coluna resulta em saltos maiores na memória, reduzindo o aproveitamento da cache e tornando o acesso mais lento.

## Analogia para Row Major vs Column Major

Pense em uma matriz armazenada na memória como uma sequência de casas com endereços consecutivos. No row major, os elementos de uma linha estão em endereços físicos consecutivos (por exemplo, 100, 101, 102, ...). Assim, ao percorrer uma linha, você acessa endereços próximos, aproveitando a localidade espacial e a cache.

No column major, os elementos de uma coluna estariam em endereços consecutivos. Se você tentar acessar uma coluna em C (row major), precisará saltar entre endereços distantes (por exemplo, 100, 110, 120, ...), o que reduz a eficiência do cache, pois os dados não estão fisicamente próximos na memória.

Em ordenação por colunas (column major), os elementos de uma mesma coluna estão armazenados em endereços de memória consecutivos. Já em C, que utiliza ordenação por linhas (row major), os elementos de uma mesma linha ficam juntos na memória. Por isso, ao acessar uma coluna em C, o programa precisa pular entre endereços de memória distantes (por exemplo, 100, 110, 120, ...). Isso diminui a eficiência do cache, pois os dados acessados não estão próximos fisicamente na memória, tornando o acesso mais lento.
