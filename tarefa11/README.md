# Tarefa 11: Simulação de Viscosidade com OpenMP

## Compilação
```bash
gcc tarefa11.c -o tarefa11 -fopenmp -lm -O2
```

## Execução
```bash
./tarefa11
```

## Descrição
Simulação simplificada de viscosidade usando Navier-Stokes com OpenMP.
Testa diferentes schedules (static, dynamic, guided) e cláusula collapse.
