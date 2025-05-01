
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define LED_PIN_GREEN 11
#define LED_PIN_RED 13
#define BUZZER_PIN 21
#define BUTTON_PIN_A 5
#define LIGHT_TIME 3000
#define NUM_PIXELS 25
#define OUT_PIN 7
#define BUTTON_PIN_B 6

#define BUZZER_FREQUENCY 4000

bool isNormalMode = true; // Flag do modo normal

typedef enum {
    SEMAFORO_VERDE,
    SEMAFORO_AMARELO,
    SEMAFORO_VERMELHO,
    SEMAFORO_NOTURNO
} SemaforoEstado;

volatile SemaforoEstado estadoAtual = SEMAFORO_VERDE; // Estado inicial do semáforo


double SEMAFORO_VERDE_PATTERN[NUM_PIXELS][3] = {
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 0, 0},
    {0, 1, 0}, {0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
};
double SEMAFORO_AMARELO_PATTERN[NUM_PIXELS][3] = {
    {1, 1, 0}, {0, 0, 0}, {1, 1, 0}, {0, 0, 0}, {1, 1, 0},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {1, 1, 0}, {0, 0, 0}, {1, 1, 0}, {0, 0, 0}, {1, 1, 0},
    {1, 1, 0}, {0, 0, 0}, {1, 1, 0}, {0, 0, 0}, {1, 1, 0},
    {1, 1, 0}, {0, 0, 0}, {1, 1, 0}, {0, 0, 0}, {1, 1, 0},
};

double SEMAFORO_VERMELHO_PATTERN[NUM_PIXELS][3] = {
    {1, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {1, 0, 0},
    {0, 0, 0}, {1, 0, 0}, {0, 0, 0}, {1, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0}, {1, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {1, 0, 0}, {0, 0, 0}, {1, 0, 0}, {0, 0, 0},
    {1, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {1, 0, 0}
};

double SEMAFORO_NOTURNO_PATTERN[NUM_PIXELS][3] = {
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {1, 1, 0}, {1, 1, 0}, {1, 1, 0}, {0, 0, 0},
    {0, 0, 0}, {1, 1, 0}, {0, 0, 0}, {1, 1, 0}, {0, 0, 0},
    {0, 0, 0}, {1, 1, 0}, {1, 1, 0}, {1, 1, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}
};

void pwm_init_buzzer(uint pin) {
    // Configurar o pino como saída de PWM
    gpio_set_function(pin, GPIO_FUNC_PWM);

    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(pin);

    // Configurar o PWM com frequência desejada
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096)); // Divisor de clock
    pwm_init(slice_num, &config, true);

    // Iniciar o PWM no nível baixo
    pwm_set_gpio_level(pin, 0);
}

// Função para emitir um beep com duração especificada
void beep(uint pin, uint duration_ms) {
    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(pin);

    // Configurar o duty cycle para 50% (ativo)
    pwm_set_gpio_level(pin, 2048);

    // Temporização
    sleep_ms(duration_ms);

    // Desativar o sinal PWM (duty cycle 0)
    pwm_set_gpio_level(pin, 0);
}

uint32_t matrix_rgb(double r, double g, double b)
{
    unsigned char R, G, B;
    R = r * 255;
    G = g * 255;
    B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}

// rotina para acionar a matrix de leds
void pio_drawn(double desenho[][3], uint32_t valor_led, PIO pio, uint sm)
{
    for (int16_t i = 0; i < NUM_PIXELS; i++)
    {
        double r = desenho[i][0] * 0.1; // Reduz a intensidade da cor
        double g = desenho[i][1] * 0.1;
        double b = desenho[i][2] * 0.1;

        valor_led = matrix_rgb(r, g, b);
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}