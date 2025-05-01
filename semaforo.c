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
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_t ssd;
    ssd1306_init(&ssd, 128, 64, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);

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
        ssd1306_draw_string(&ssd, mode, 10, 6);        // Exibe a color
        ssd1306_draw_string(&ssd, color, 10, 16);     // Exibe o modo
        ssd1306_draw_string(&ssd, "Semaforo", 33, 28); // Exibe o título
        ssd1306_draw_string(&ssd, message, 10, 41); // Exibe a mensagem
        ssd1306_draw_string(&ssd, message2, 10, 52); // Exibe a mensagem

        // Atualiza o display
        ssd1306_send_data(&ssd);

        vTaskDelay(pdMS_TO_TICKS(1000)); // Atualiza a cada 1 segundo
    }
}

void vButtonTask()
{
    gpio_init(BUTTON_PIN_A);
    gpio_set_dir(BUTTON_PIN_A, GPIO_IN);
    gpio_pull_up(BUTTON_PIN_A);
    gpio_init(BUTTON_PIN_B);
    gpio_set_dir(BUTTON_PIN_B, GPIO_IN);
    gpio_pull_up(BUTTON_PIN_B);
    while(true)
    {
        if(!gpio_get(BUTTON_PIN_A))
        {
            isNormalMode = !isNormalMode;
            vTaskDelay(pdMS_TO_TICKS(500)); 
        }
        if(!gpio_get(BUTTON_PIN_B))
        {
            // Se o botão B for pressionado, reinicia o modo BOOTSEL
            printf("BOOTSEL\n");
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
                for (int i = 0; i < 5; i++) {
                    beep(BUZZER_PIN, 100); // 100ms
                    vTaskDelay(pdMS_TO_TICKS(100)); // Pausa de 100ms
                }
                break;

            case SEMAFORO_VERMELHO:
                // Beep contínuo curto para vermelho
                beep(BUZZER_PIN, 500); // 500ms
                vTaskDelay(pdMS_TO_TICKS(LIGHT_TIME - 500)); // Sincroniza com o tempo restante
                break;

            case SEMAFORO_NOTURNO:
                // Beep lento no modo noturno
                beep(BUZZER_PIN, 500); // 500ms
                vTaskDelay(pdMS_TO_TICKS(1500)); // 1.5s de pausa
                break;
        }
    }
}

void vLedTask() {
    gpio_init(LED_PIN_GREEN);
    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

    while (true) {
        if (isNormalMode)
        {
            // Verde
            estadoAtual = SEMAFORO_VERDE;
            gpio_put(LED_PIN_GREEN, true);
            gpio_put(LED_PIN_RED, false);
            vTaskDelay(pdMS_TO_TICKS(LIGHT_TIME)); 
            // Amarelo (Verde + Vermelho)
            estadoAtual = SEMAFORO_AMARELO;
            gpio_put(LED_PIN_GREEN, true);
            gpio_put(LED_PIN_RED, true);
            vTaskDelay(pdMS_TO_TICKS(LIGHT_TIME));

            // Vermelho
            estadoAtual = SEMAFORO_VERMELHO;
            gpio_put(LED_PIN_GREEN, false);
            gpio_put(LED_PIN_RED, true);
            vTaskDelay(pdMS_TO_TICKS(LIGHT_TIME));
        }
        else
        {
            estadoAtual = SEMAFORO_NOTURNO;
            gpio_put(LED_PIN_GREEN, true);
            gpio_put(LED_PIN_RED, true);
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_put(LED_PIN_GREEN, false);
            gpio_put(LED_PIN_RED, false);
            vTaskDelay(pdMS_TO_TICKS(1500));
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
        vTaskDelay(pdMS_TO_TICKS(10)); // Atualiza o padrão a cada 100ms
    }
}

int main()
{
    stdio_init_all();

    xTaskCreate(vButtonTask, "Button Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vLedTask, "LED Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vBuzzerTask, "Buzzer Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vDisplayTask, "Display Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vMatrixTask, "Matrix Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();
    panic_unsupported();
}
