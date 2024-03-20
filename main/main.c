/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "ssd1306.h"
#include "gfx.h"

#include "pico/stdlib.h"
#include <stdio.h>

const uint BTN_1_OLED = 28;
const uint BTN_2_OLED = 26;
const uint BTN_3_OLED = 27;

const uint LED_1_OLED = 20;
const uint LED_2_OLED = 21;
const uint LED_3_OLED = 22;

int ECHO_PIN = 13; 
int TRIG_PIN = 16;

SemaphoreHandle_t xSemaphore_trig;

QueueHandle_t xQueueButId;

void oled1_btn_led_init(void) {
    gpio_init(LED_1_OLED);
    gpio_set_dir(LED_1_OLED, GPIO_OUT);

    gpio_init(LED_2_OLED);
    gpio_set_dir(LED_2_OLED, GPIO_OUT);

    gpio_init(LED_3_OLED);
    gpio_set_dir(LED_3_OLED, GPIO_OUT);

    gpio_init(BTN_1_OLED);
    gpio_set_dir(BTN_1_OLED, GPIO_IN);
    gpio_pull_up(BTN_1_OLED);

    gpio_init(BTN_2_OLED);
    gpio_set_dir(BTN_2_OLED, GPIO_IN);
    gpio_pull_up(BTN_2_OLED);

    gpio_init(BTN_3_OLED);
    gpio_set_dir(BTN_3_OLED, GPIO_IN);
    gpio_pull_up(BTN_3_OLED);
}

void oled1_demo_1(void *p) {
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();

    char cnt = 15;
    while (1) {

        if (gpio_get(BTN_1_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_1_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 1 - ON");
            gfx_show(&disp);
        } else if (gpio_get(BTN_2_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_2_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 2 - ON");
            gfx_show(&disp);
        } else if (gpio_get(BTN_3_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_3_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 3 - ON");
            gfx_show(&disp);
        } else {

            gpio_put(LED_1_OLED, 1);
            gpio_put(LED_2_OLED, 1);
            gpio_put(LED_3_OLED, 1);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "PRESSIONE ALGUM");
            gfx_draw_string(&disp, 0, 10, 1, "BOTAO");
            gfx_draw_line(&disp, 15, 27, cnt,
                          27);
            vTaskDelay(pdMS_TO_TICKS(50));
            if (++cnt == 112)
                cnt = 15;

            gfx_show(&disp);
        }
    }
}

void led_task(void *p) {
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();

    char string_double[20];

    while (1) {
        
        double d;
        if (xSemaphoreTake(xSemaphore_trig, pdMS_TO_TICKS(500)) == pdTRUE) {
            if (xQueueReceive(xQueueButId, &d,  pdMS_TO_TICKS(100))) {
                printf("Entro na fila \n");
                // Transforma double em str
                sprintf(string_double, "%.2f cm", d);
                printf("String double é %s \n", string_double);
            }

            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 2, string_double);
            gfx_show(&disp);
            vTaskDelay(pdMS_TO_TICKS(150));
            
        } else{
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 2, "Falha");
            gfx_show(&disp);
            vTaskDelay(pdMS_TO_TICKS(150));
        }

    }

    
}


void trigger_task(void *p) {
    printf("trigger task \n");
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_put(TRIG_PIN, 0);

    int delay = 250;

    while (true) {
        
        gpio_put(TRIG_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(delay));
        gpio_put(TRIG_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(delay));
        
    }
}

void echo_task(void *p) {
    printf("echo task \n");
    uint32_t start_us = 0;
    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    gpio_pull_up(ECHO_PIN);

    while (true) {
        if (!gpio_get(ECHO_PIN)) { // se echo==0
            while (!gpio_get(ECHO_PIN)) { // espera echo ser 1
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            start_us = to_us_since_boot(get_absolute_time());
        }   
        if (gpio_get(ECHO_PIN)) { // se echo==1
            while (gpio_get(ECHO_PIN)) { // espera echo ser 0
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            uint32_t delta_t = to_us_since_boot(get_absolute_time()) - start_us;
            double distancia =  (340* delta_t/10000)/2.0;
            printf("%lf cm \n", distancia);
            xQueueSend(xQueueButId, &distancia, 0);
            xSemaphoreGive(xSemaphore_trig);
        }
    }
 
}

int main() {
    stdio_init_all();

    xQueueButId = xQueueCreate(32, sizeof(double));
    xSemaphore_trig = xSemaphoreCreateBinary();

    xTaskCreate(trigger_task, "Trigger_task", 256, NULL, 1, NULL);
    xTaskCreate(echo_task, "Echo_task", 256, NULL, 1, NULL);
    xSemaphoreGive(xSemaphore_trig);

    xTaskCreate(led_task, "Led_task", 4095, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
