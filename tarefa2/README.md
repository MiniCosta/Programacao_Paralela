# Como compilar e executar

Compile o programa com diferentes níveis de otimização e compare os tempos de execução:

Linux:
```
gcc tarefa2.c -O0 -o tarefa2_O0
gcc tarefa2.c -O2 -o tarefa2_O2
gcc tarefa2.c -O3 -o tarefa2_O3
```

Windows (MinGW):
```
gcc tarefa2.c -O0 -o tarefa2_O0.exe
gcc tarefa2.c -O2 -o tarefa2_O2.exe
gcc tarefa2.c -O3 -o tarefa2_O3.exe
```

Execute e observe os tempos:
```
./tarefa2_O0
./tarefa2_O2
./tarefa2_O3
```

# Análise
- O laço 1 (inicialização) não tem dependências e pode ser otimizado facilmente pelo compilador.
- O laço 2 (soma acumulativa) tem dependência entre iterações, limitando o paralelismo.
- O laço 3 (soma com múltiplas variáveis) quebra a dependência, permitindo maior paralelismo e melhor uso de ILP.
- Espere ganhos de desempenho maiores nos laços 1 e 3 com otimizações (-O2, -O3), enquanto o laço 2 será menos beneficiado.
