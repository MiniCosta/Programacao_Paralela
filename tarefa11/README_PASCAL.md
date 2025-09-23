# 🌊 Simulação Navier-Stokes com OpenMP e PaScal Suite

## 📋 Visão Geral

Este projeto implementa uma simulação simplificada da equação de Navier-Stokes (apenas viscosidade) com instrumentação PaScal para análise detalhada de escalabilidade. O código foi incrementado com base nas especificações do [PaScal Suite Analyzer](https://pascalsuite.imd.ufrn.br/analyzer/#key-features).

## 🚀 Funcionalidades

- ✅ **Simulação Navier-Stokes** com OpenMP
- ✅ **Instrumentação PaScal** manual com 9 regiões
- ✅ **Análise de escalabilidade** (2, 4, 8 cores)
- ✅ **Comparação de estratégias** (schedule static vs collapse)
- ✅ **Compilação condicional** (com/sem PaScal)
- ✅ **Interface visual** com análise automática

## 🔧 Instalação e Configuração

### Pré-requisitos
- Linux x86_64
- GCC com suporte OpenMP
- PaScal Suite (opcional)

### Instalação do PaScal Suite
```bash
wget -c https://gitlab.com/lappsufrn/pascal-releases/-/archive/master/pascal-releases-master.zip
unzip pascal-releases-master.zip
cd pascal-releases-master/
source env.sh
```

## 📊 Regiões de Instrumentação

| Região | Descrição |
|--------|-----------|
| 100 | Programa completo |
| 1 | Simulação serial completa |
| 11 | Loop principal serial |
| 12 | Cópia de dados serial |
| 2 | Simulação paralela static completa |
| 21 | Loop principal paralelo static |
| 22 | Cópia de dados paralela static |
| 3 | Simulação paralela collapse completa |
| 31 | Loop principal paralelo collapse |
| 32 | Cópia de dados paralela collapse |

## 🛠️ Compilação

### Versão Básica (sem PaScal)
```bash
gcc -O2 -fopenmp tarefa11_simples.c -o tarefa11_simples_basic -lm
```

### Versão Instrumentada (com PaScal)
```bash
gcc -O2 -fopenmp -DUSE_PASCAL tarefa11_simples.c -o tarefa11_simples_pascal -lm -lmpascalops
```

### Script Automático
```bash
./run_analysis.sh [grid_size] [iterations]
```

## 🎯 Execução

### Execução Básica
```bash
./tarefa11_simples_basic 256 500
```

### Análise com PaScal Analyzer
```bash
# Executar uma vez para gerar dados
./tarefa11_simples_pascal 256 500

# Análise automática com múltiplas configurações
pascalanalyzer ./tarefa11_simples_pascal --inst man --cors 2,4,8 --ipts "128 200","256 500","512 1000" --verb INFO
```

### Exemplos de Comando PaScal
```bash
# Análise básica
pascalanalyzer ./tarefa11_simples_pascal --inst man --cors 2,4 --ipts "256 500" --verb INFO

# Análise completa com repetições
pascalanalyzer ./tarefa11_simples_pascal --inst man --cors 1,2,4,8 --ipts "128 200","256 500","512 1000" --rpts 3 --outp escalabilidade.json --verb INFO

# Análise com frequências
pascalanalyzer ./tarefa11_simples_pascal --inst man --cors 2,4 --frqs 2000000,2500000 --ipts "256 500" --verb DEBUG
```

## 📈 Análise de Resultados

### Saída do Programa
O programa gera automaticamente:
- Tempos de execução para cada configuração
- Cálculos de speedup e eficiência
- Comparação entre estratégias de paralelização
- Estatísticas de performance (MFLOPS, iterações/segundo)

### Dados PaScal
Com PaScal habilitado, são coletados:
- Tempos detalhados por região
- Consumo de energia (se habilitado)
- Contadores de performance
- Dados para análise visual no PaScal Viewer

## 🔍 Estrutura do Código

```
tarefa11_simples.c
├── Cabeçalho com instruções
├── Includes condicionais (PaScal)
├── Parâmetros configuráveis
├── Funções de alocação de matriz
├── Função Laplaciano
├── Condições de contorno
├── simulate_serial() [Regiões 1, 11, 12]
├── simulate_parallel_static() [Regiões 2, 21, 22]
├── simulate_parallel_collapse() [Regiões 3, 31, 32]
└── main() [Região 100]
```

## 📊 Parâmetros de Simulação

| Parâmetro | Descrição | Valor Padrão |
|-----------|-----------|--------------|
| N | Tamanho da grade (NxN) | 256 |
| ITER | Número de iterações | 500 |
| DT | Passo temporal | 0.00001 |
| NU | Viscosidade cinemática | 0.1 |

## 🎯 Casos de Uso

### Desenvolvimento Local
```bash
# Teste rápido sem PaScal
./run_analysis.sh 128 100
```

### Análise de Escalabilidade
```bash
# Com PaScal para análise detalhada
pascalanalyzer ./tarefa11_simples_pascal --inst man --cors 1,2,4,8 --ipts "256 500","512 1000" --rpts 2 --outp results.json
```

### Análise de Performance
```bash
# Incluir contadores de performance
pascalanalyzer ./tarefa11_simples_pascal --inst man --cors 2,4 --ipts "512 1000" --fgpe cache-misses,instructions --verb INFO
```

## 🧪 Resultados Esperados

### Escalabilidade Típica
- **2 cores**: Speedup ~1.5-1.8x, Eficiência 75-90%
- **4 cores**: Speedup ~2.0-2.6x, Eficiência 50-65%
- **8 cores**: Speedup limitado, Eficiência <40%

### Schedule Static vs Collapse
- **Static**: Geralmente superior para este algoritmo
- **Collapse**: Pode ter overhead adicional em grids pequenos

## 📁 Arquivos Gerados

- `tarefa11_simples_basic` - Executável sem PaScal
- `tarefa11_simples_pascal` - Executável com PaScal
- `results.json` - Dados PaScal (se especificado)
- `run_analysis.sh` - Script de execução automática

## 🚨 Resolução de Problemas

### Erro de Compilação com PaScal
```bash
# Verificar se PaScal está instalado
which pascalanalyzer

# Verificar variáveis de ambiente
echo $PASCAL_DIR
```

### Performance Inesperada
- Grids muito pequenos sofrem com overhead de paralelização
- 8+ cores podem ter contenção de memória
- Use `--verb DEBUG` para análise detalhada

## 📚 Referências

- [PaScal Suite Official](https://pascalsuite.imd.ufrn.br/)
- [PaScal Analyzer Features](https://pascalsuite.imd.ufrn.br/analyzer/#key-features)
- [OpenMP Specification](https://www.openmp.org/)

---
*Desenvolvido para análise de escalabilidade em programação paralela com instrumentação PaScal Suite.*

Saída para:
./tarefa11_simples_pascal --inst man --cors 2,4,6,8 --ipts "128 100","1024 400" --outp pascal
_analysis.json --verb INFO

[Pascal] Running with ['cores', 'input', 'repetitions']
[Pascal] Cores: 8 Input: 0 
[Pascal] Total time 0.10766744613647461
[Pascal] Cores: 8 Input: 1 
[Pascal] Total time 21.241076231002808
[Pascal] Cores: 6 Input: 0 
[Pascal] Total time 0.11360740661621094
[Pascal] Cores: 6 Input: 1 
[Pascal] Total time 20.634087085723877
[Pascal] Cores: 4 Input: 0 
[Pascal] Total time 0.11514592170715332
[Pascal] Cores: 4 Input: 1 
[Pascal] Total time 20.63106393814087
[Pascal] Cores: 2 Input: 0 
[Pascal] Total time 0.11112737655639648
[Pascal] Cores: 2 Input: 1 
[Pascal] Total time 21.48866105079651
[Pascal] ***** Done! *****
[Pascal] Saving data