#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "pio_matrix.pio.h"
#include "config.h"
#include <stdio.h>
#include "pico/bootrom.h"

void vDisplayTask() {
    // Inicializa o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_t ssd;
    ssd1306_init(&ssd, 128, 64, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);

    // Variáveis para armazenar informações do semáforo
    char color[16];
    char mode[16];
    char message[32];
    char message2[32];

    while (true) {
        ssd1306_fill(&ssd, false); // Limpa o display

        // Desenha o retângulo e as linhas
        ssd1306_rect(&ssd, 3, 3, 122, 60, true, false);      // Desenha um retângulo
        ssd1306_line(&ssd, 3, 25, 123, 25, true);           // Desenha uma linha
        ssd1306_line(&ssd, 3, 37, 123, 37, true);           // Desenha uma linha

        // Define as informações com base no estado atual
        switch (estadoAtual) {
            case SEMAFORO_VERDE:
                sprintf(color, "Cor: Verde");
                sprintf(mode, "Modo: Normal");
                sprintf(message, "Pode");
                sprintf(message2, "atravessar!");
                break;

            case SEMAFORO_AMARELO:
                sprintf(color, "Cor: Amarelo");
                sprintf(mode, "Modo: Normal");
                sprintf(message, "Atencao!");
                sprintf(message2, "");

                break;

            case SEMAFORO_VERMELHO:
                sprintf(color, "Cor: Vermelho");
                sprintf(mode, "Modo: Normal");
                sprintf(message, "Pare!");
                sprintf(message2, "");
                break;

            case SEMAFORO_NOTURNO:
                sprintf(color, "Cor: Amarelo");
                sprintf(mode, "Modo: Noturno");
                sprintf(message, "Cuidado ao");
                sprintf(message2, "atravessar!");
                break;
        }

        // Exibe as informações no display
        ssd1306_draw_string(&ssd, mode, 10, 6);        // Exibe o modo
        ssd1306_draw_string(&ssd, color, 10, 16);     // Exibe a cor
        ssd1306_draw_string(&ssd, "Semaforo", 33, 28); // Exibe o título
        ssd1306_draw_string(&ssd, message, 10, 41); // Exibe a mensagem
        ssd1306_draw_string(&ssd, message2, 10, 52); // Exibe a mensagem2

        // Atualiza o display
        ssd1306_send_data(&ssd);

        vTaskDelay(pdMS_TO_TICKS(500)); // Atualiza a cada 0.5 segundo
    }
}

void vButtonTask()
{
    // Inicializa os pinos dos botões
    gpio_init(BUTTON_PIN_A);
    gpio_set_dir(BUTTON_PIN_A, GPIO_IN);
    gpio_pull_up(BUTTON_PIN_A);
    gpio_init(BUTTON_PIN_B);
    gpio_set_dir(BUTTON_PIN_B, GPIO_IN);
    gpio_pull_up(BUTTON_PIN_B);
    while(true)
    {
        // Verifica o estado dos botões
        // Se o botão A for pressionado, alterna entre os modos normal e noturno
        if(!gpio_get(BUTTON_PIN_A))
        {
            isNormalMode = !isNormalMode;
            vTaskDelay(pdMS_TO_TICKS(500)); // Debounce de 500ms
        }
        // Se o botão B for pressionado, reinicia o modo BOOTSEL
        if(!gpio_get(BUTTON_PIN_B))
        {
            reset_usb_boot(0, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}

void vBuzzerTask() {
    pwm_init_buzzer(BUZZER_PIN);

    while (true) {
        switch (estadoAtual) {
            case SEMAFORO_VERDE:
                // Beep longo para verde
                beep(BUZZER_PIN, 1000); // 1 segundo
                vTaskDelay(pdMS_TO_TICKS(LIGHT_TIME - 1000)); // Sincroniza com o tempo restante
                break;
            case SEMAFORO_AMARELO:
                // Beeps curtos e rápidos para amarelo
                beep(BUZZER_PIN, 100); // 100ms
                vTaskDelay(pdMS_TO_TICKS(100)); // Pausa de 100ms
                break;
            case SEMAFORO_VERMELHO:
                // Beeps contínuo  para vermelho
                beep(BUZZER_PIN, 500); // 500ms
                vTaskDelay(pdMS_TO_TICKS(1500)); //Pausa de 1,5 segundos
                break;
            case SEMAFORO_NOTURNO:
                // Beep lento no modo noturno
                beep(BUZZER_PIN, 300); // 300ms
                vTaskDelay(pdMS_TO_TICKS(2000)); // Pausa de 2 segundos
                break;
        }
    }
}

void vLedTask() {
    // Inicializa os pinos dos LEDs
    gpio_init(LED_PIN_GREEN);
    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

    while (true) {
        switch (estadoAtual) {
            case SEMAFORO_VERDE:
                // Verde
                gpio_put(LED_PIN_GREEN, true);
                gpio_put(LED_PIN_RED, false);
                vTaskDelay(pdMS_TO_TICKS(LIGHT_TIME)); 
    
                // Transição para Amarelo
                if (isNormalMode) {
                    estadoAtual = SEMAFORO_AMARELO;
                } 
                else
                {
                    estadoAtual = SEMAFORO_NOTURNO;
                }
                
                break;
    
            case SEMAFORO_AMARELO:
                // Amarelo (Verde + Vermelho)
                gpio_put(LED_PIN_GREEN, true);
                gpio_put(LED_PIN_RED, true);
                vTaskDelay(pdMS_TO_TICKS(LIGHT_TIME));
    
                // Transição para Vermelho
                if (isNormalMode) {
                    estadoAtual = SEMAFORO_VERMELHO;
                } 
                else
                {
                    estadoAtual = SEMAFORO_NOTURNO;
                }
                break;
    
            case SEMAFORO_VERMELHO:
                // Vermelho
                gpio_put(LED_PIN_GREEN, false);
                gpio_put(LED_PIN_RED, true);
                vTaskDelay(pdMS_TO_TICKS(LIGHT_TIME));
                // Transição para Verde
                if (isNormalMode) {
                    estadoAtual = SEMAFORO_VERDE;
                } 
                else
                {
                    estadoAtual = SEMAFORO_NOTURNO;
                }
                break;
    
            case SEMAFORO_NOTURNO:
                // Modo Noturno (Amarelo piscando)
                gpio_put(LED_PIN_GREEN, true);
                gpio_put(LED_PIN_RED, true);
                vTaskDelay(pdMS_TO_TICKS(300));
                gpio_put(LED_PIN_GREEN, false);
                gpio_put(LED_PIN_RED, false);
                vTaskDelay(pdMS_TO_TICKS(2000));
                if (isNormalMode) {
                    estadoAtual = SEMAFORO_VERDE;
                }
                // Permanece no modo noturno até alteração pelo botão
                break; 
        }
    }
}

void vMatrixTask() {
    // Inicializa o PIO e carrega o programa
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);

    while (true) {
        // Atualiza o padrão de LEDs com base no estado atual
        switch (estadoAtual) {
            case SEMAFORO_VERDE:
                pio_drawn(SEMAFORO_VERDE_PATTERN, 0, pio, sm);
                break;

            case SEMAFORO_AMARELO:
                pio_drawn(SEMAFORO_AMARELO_PATTERN, 0, pio, sm);
                break;

            case SEMAFORO_VERMELHO:
                pio_drawn(SEMAFORO_VERMELHO_PATTERN, 0, pio, sm);
                break;

            case SEMAFORO_NOTURNO:
                pio_drawn(SEMAFORO_NOTURNO_PATTERN, 0, pio, sm);
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Atualiza o padrão a cada 100ms
    }
}

int main()
{
    stdio_init_all();

    //Cria as tarefas
    xTaskCreate(vButtonTask, "Button Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vLedTask, "LED Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vBuzzerTask, "Buzzer Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vDisplayTask, "Display Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vMatrixTask, "Matrix Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler(); // Inicia o escalonador FreeRTOS
    panic_unsupported();
}
