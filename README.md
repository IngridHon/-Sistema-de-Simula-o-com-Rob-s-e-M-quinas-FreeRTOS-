#  Sistema de Simulação com Robôs e Máquinas (FreeRTOS)

Este projeto implementa um sistema de simulação com **robôs** e **máquinas** utilizando o **FreeRTOS**, onde tarefas representam diferentes agentes industriais que manipulam itens através de filas e semáforos.

---

##  Visão Geral

O sistema simula um processo produtivo automatizado com:

- Robôs (`R1` a `R4`)
- Máquinas (`M1`, `M2`, `M3`)
- Depósitos intermediários modelados com **filas (`QueueHandle_t`)**
- Controle de acesso concorrente utilizando **semáforos (`SemaphoreHandle_t`)**

---

##  Funcionalidade das Tarefas

###  Robôs

- **R1**: Retira insumos do depósito de entrada e coloca na máquina M1.
- **R2**: Transporta itens de M1 para M2, com controle de acesso via semáforo.
- **R3**: Transporta itens de M1 para M3, também com semáforo.
- **R4**: Retira itens de M2 ou M3 e leva para o depósito de saída.

###  Máquinas

- **M1**: Produz novos itens a cada 1.5 segundos.
- **M2** e **M3**: Recebem itens para processamento e os descartam após um tempo específico.

---

## Tempos de Operação

| Componente            | Tempo (ms) |
|-----------------------|------------|
| Movimento R1, R2, R4  | 500        |
| Movimento R3          | 800        |
| Operações (pegar/colocar item) | 100 |
| Produção M1/M2        | 1500       |
| Produção M3           | 3000       |

---

##  Tecnologias e Recursos Usados

- Linguagem: `C`
- Sistema Operacional: [FreeRTOS](https://freertos.org)
- Conceitos principais:
  - Programação concorrente
  - Filas e Semáforos
  - Simulação de célula de manufatura

---

##  Estrutura de Recursos

```c
QueueHandle_t depositoM1;        // Depósito entre R1 e R2/R3
QueueHandle_t depositoM2;        // Depósito entre R2 e M2
QueueHandle_t depositoM3;        // Depósito entre R3 e M3
QueueHandle_t depositoSaida;     // Depósito final após R4

SemaphoreHandle_t semaforoM1;        // Controle de acesso para R2 e R3 ao depósito M1
SemaphoreHandle_t semaforoEntregaR4; // Controle de acesso para R4 aos depósitos M2 e M3

```
Autor: Ingrid Honório da Silva - 119210830
Guilherme Santos da Silveira - 124212387

---- Vídeo de demonstração no youtube
[https://youtu.be/ssoxLamEsm8](https://www.youtube.com/watch?v=ssoxLamEsm8)
