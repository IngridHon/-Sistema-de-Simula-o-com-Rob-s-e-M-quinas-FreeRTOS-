#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// Definições dos tempos em milissegundos
#define TEMPO_MOV_R1_R2 500  // 0.5s em ms (movimentação de R1, R2 e R4)
#define TEMPO_MOV_R3 800     // 0.8s em ms (movimentação de R3)
#define TEMPO_OPERACAO 100   // 0.1s em ms (tempo de retirada/colocação de item)
#define TEMPO_PROD_M1_M2 1500 // 1.5s em ms (produção das máquinas M1 e M2)
#define TEMPO_PROD_M3 3000   // 3s em ms (produção da máquina M3)

// Filas para simular os depósitos das máquinas
QueueHandle_t depositoM1;
QueueHandle_t depositoM2;
QueueHandle_t depositoM3;
QueueHandle_t depositoSaida;

// Semáforos para controle de acesso aos depósitos compartilhados
SemaphoreHandle_t semaforoM1;
SemaphoreHandle_t semaforoEntregaR4;

/* Provide memory for the idle task when static allocation is enabled. */
void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer,
    StackType_t** ppxIdleTaskStackBuffer,
    uint32_t* pulIdleTaskStackSize)
{
    static StaticTask_t xIdleTaskTCB;
    static StackType_t xIdleTaskStack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = xIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/* Provide memory for the timer task when static allocation is enabled. */
void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer,
    StackType_t** ppxTimerTaskStackBuffer,
    uint32_t* pulTimerTaskStackSize)
{
    static StaticTask_t xTimerTaskTCB;
    static StackType_t xTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = xTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

void vAssertCalled(const char* file, unsigned long line)
{
    printf("Asserção falhou no arquivo %s na linha %lu\n", file, line);
    taskDISABLE_INTERRUPTS();
    for (;;)
        ;
}

// Robô 1: Retira insumos do depósito de entrada da célula e coloca em M1
void robo1(void* pvParameters)
{
    (void)pvParameters;
    printf("R1: Tarefa iniciada.\n");
    while (1)
    {
        printf("R1: Retirando item do depósito de entrada.\n");
        vTaskDelay(pdMS_TO_TICKS(TEMPO_OPERACAO));
        char item[] = "Item";
        xQueueSend(depositoM1, &item, portMAX_DELAY);
        printf("R1: Item enviado para M1.\n");
        vTaskDelay(pdMS_TO_TICKS(TEMPO_MOV_R1_R2));
    }
}

// Robô 2: Retira itens de M1 e leva para M2
void robo2(void* pvParameters)
{
    (void)pvParameters;
    printf("R2: Tarefa iniciada.\n");
    while (1)
    {
        if (xSemaphoreTake(semaforoM1, portMAX_DELAY))
        {
            printf("R2: Acesso ao depósito M1 garantido.\n");
            char item[10];
            if (xQueueReceive(depositoM1, &item, portMAX_DELAY))
            {
                printf("R2: Item retirado de M1.\n");
                vTaskDelay(pdMS_TO_TICKS(TEMPO_OPERACAO));
                xQueueSend(depositoM2, &item, portMAX_DELAY);
                printf("R2: Item enviado para M2.\n");
            }
            xSemaphoreGive(semaforoM1);
            printf("R2: Acesso ao depósito M1 liberado.\n");
        }
        vTaskDelay(pdMS_TO_TICKS(TEMPO_MOV_R1_R2));
    }
}

// Robô 3: Retira itens de M1 e leva para M3
void robo3(void* pvParameters)
{
    (void)pvParameters;
    printf("R3: Tarefa iniciada.\n");
    while (1)
    {
        if (xSemaphoreTake(semaforoM1, portMAX_DELAY))
        {
            printf("R3: Acesso ao depósito M1 garantido.\n");
            char item[10];
            if (xQueueReceive(depositoM1, &item, portMAX_DELAY))
            {
                printf("R3: Item retirado de M1.\n");
                vTaskDelay(pdMS_TO_TICKS(TEMPO_OPERACAO));
                xQueueSend(depositoM3, &item, portMAX_DELAY);
                printf("R3: Item enviado para M3.\n");
            }
            xSemaphoreGive(semaforoM1);
            printf("R3: Acesso ao depósito M1 liberado.\n");
        }
        vTaskDelay(pdMS_TO_TICKS(TEMPO_MOV_R3));
    }
}

// Robô 4: Retira itens de M2 e M3 e leva para o depósito de saída
void robo4(void* pvParameters)
{
    (void)pvParameters;
    printf("R4: Tarefa iniciada.\n");
    while (1)
    {
        xSemaphoreTake(semaforoEntregaR4, portMAX_DELAY);
        printf("R4: Acesso garantido para entrega.\n");
        char item[10];
        if (xQueueReceive(depositoM2, &item, 0))
        {
            printf("R4: Item retirado de M2.\n");
            vTaskDelay(pdMS_TO_TICKS(TEMPO_OPERACAO));
            xQueueSend(depositoSaida, &item, portMAX_DELAY);
            printf("R4: Item enviado para o depósito de saída.\n");
        }
        else if (xQueueReceive(depositoM3, &item, 0))
        {
            printf("R4: Item retirado de M3.\n");
            vTaskDelay(pdMS_TO_TICKS(TEMPO_OPERACAO));
            xQueueSend(depositoSaida, &item, portMAX_DELAY);
            printf("R4: Item enviado para o depósito de saída.\n");
        }
        xSemaphoreGive(semaforoEntregaR4);
        printf("R4: Acesso para entrega liberado.\n");
        vTaskDelay(pdMS_TO_TICKS(TEMPO_MOV_R1_R2));
    }
}

// Máquina 1: Produz um item a cada 1.5s
void maquina1(void* pvParameters)
{
    (void)pvParameters;
    printf("M1: Tarefa iniciada.\n");
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(TEMPO_PROD_M1_M2));
        char item[] = "Item";
        if (xQueueSend(depositoM1, &item, portMAX_DELAY) == pdPASS)
        {
            printf("M1: Item produzido e enviado para o depósito M1.\n");
        }
        else
        {
            printf("M1: Falha ao enviar item para o depósito M1.\n");
        }
    }
}

// Máquina 2: Processa itens e os descarta
void maquina2(void* pvParameters)
{
    (void)pvParameters;
    printf("M2: Tarefa iniciada.\n");
    while (1)
    {
        char item[10];
        if (xQueueReceive(depositoM2, &item, portMAX_DELAY))
        {
            printf("M2: Item recebido para processamento.\n");
            vTaskDelay(pdMS_TO_TICKS(TEMPO_PROD_M1_M2));
            printf("M2: Item processado e descartado.\n");
        }
    }
}

// Máquina 3: Processa itens e os descarta
void maquina3(void* pvParameters)
{
    (void)pvParameters;
    printf("M3: Tarefa iniciada.\n");
    while (1)
    {
        char item[10];
        if (xQueueReceive(depositoM3, &item, portMAX_DELAY))
        {
            printf("M3: Item recebido para processamento.\n");
            vTaskDelay(pdMS_TO_TICKS(TEMPO_PROD_M3));
            printf("M3: Item processado e descartado.\n");
        }
    }
}

int main()
{
    // Criação das filas para os depósitos
    depositoM1 = xQueueCreate(5, sizeof(char[10]));
    depositoM2 = xQueueCreate(5, sizeof(char[10]));
    depositoM3 = xQueueCreate(5, sizeof(char[10]));
    depositoSaida = xQueueCreate(10, sizeof(char[10]));

    if (depositoM1 == NULL || depositoM2 == NULL || depositoM3 == NULL || depositoSaida == NULL)
    {
        printf("Erro ao criar as filas.\n");
        return 1;
    }

    // Criação dos semáforos para controle de acesso
    semaforoM1 = xSemaphoreCreateMutex();
    semaforoEntregaR4 = xSemaphoreCreateMutex();

    if (semaforoM1 == NULL || semaforoEntregaR4 == NULL)
    {
        printf("Erro ao criar os semáforos.\n");
        return 1;
    }

    // Criação das tarefas para robôs e máquinas
    if (xTaskCreate(robo1, "R1", configMINIMAL_STACK_SIZE * 2, NULL, 2, NULL) != pdPASS)
    {
        printf("Erro ao criar a tarefa R1.\n");
    }
    if (xTaskCreate(robo2, "R2", configMINIMAL_STACK_SIZE * 2, NULL, 2, NULL) != pdPASS)
    {
        printf("Erro ao criar a tarefa R2.\n");
    }
    if (xTaskCreate(robo3, "R3", configMINIMAL_STACK_SIZE * 2, NULL, 2, NULL) != pdPASS)
    {
        printf("Erro ao criar a tarefa R3.\n");
    }
    if (xTaskCreate(robo4, "R4", configMINIMAL_STACK_SIZE * 2, NULL, 3, NULL) != pdPASS)
    {
        printf("Erro ao criar a tarefa R4.\n");
    }
    if (xTaskCreate(maquina1, "M1", configMINIMAL_STACK_SIZE * 2, NULL, 2, NULL) != pdPASS)
    {
        printf("Erro ao criar a tarefa M1.\n");
    }
    if (xTaskCreate(maquina2, "M2", configMINIMAL_STACK_SIZE * 2, NULL, 2, NULL) != pdPASS)
    {
        printf("Erro ao criar a tarefa M2.\n");
    }
    if (xTaskCreate(maquina3, "M3", configMINIMAL_STACK_SIZE * 2, NULL, 2, NULL) != pdPASS)
    {
        printf("Erro ao criar a tarefa M3.\n");
    }

    printf("Iniciando o escalonador...\n");
    vTaskStartScheduler();
    printf("Erro: O escalonador não foi iniciado.\n");
    return 0;
}
