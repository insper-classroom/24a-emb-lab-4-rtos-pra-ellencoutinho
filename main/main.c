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

QueueHandle_t xQueueDistance;
QueueHandle_t xQueueTime;

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
            if (xQueueReceive(xQueueDistance, &d,  pdMS_TO_TICKS(100))) {
                // Transforma double em str
                sprintf(string_double, "%.2f cm", d);
            }

            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 2, string_double);
            if(d<112){
                gfx_draw_line(&disp, 15, 27, (int)d, 27);
            }
            else{
                gfx_draw_line(&disp, 15, 27, 112, 27);
            }
            vTaskDelay(pdMS_TO_TICKS(50));
            
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

void gpio_callback(uint gpio, uint32_t events){
    uint32_t time;
    if (events == GPIO_IRQ_EDGE_RISE) {
        time=to_us_since_boot(get_absolute_time());
        
    } else if(events==GPIO_IRQ_EDGE_FALL){
        time=to_us_since_boot(get_absolute_time());
        
    }
    xQueueSendFromISR(xQueueTime, &time, 0);
        

}

void echo_task(void *p) {
    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(ECHO_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true,
                                       &gpio_callback);

    while (true) {
        uint32_t start_us;
        uint32_t end_us;
        if (xQueueReceive(xQueueTime, &start_us,  pdMS_TO_TICKS(100))){
            if (xQueueReceive(xQueueTime, &end_us,  pdMS_TO_TICKS(100))) {
                double distancia =  (340* (end_us - start_us)/10000)/2.0;
                printf("%lf cm \n", distancia);
                xQueueSend(xQueueDistance, &distancia, 0);
                xSemaphoreGive(xSemaphore_trig);
            }
        }
    }
    
}

int main() {
    stdio_init_all();

    xQueueDistance = xQueueCreate(32, sizeof(double));
    xQueueTime = xQueueCreate(32, sizeof(double));
    xSemaphore_trig = xSemaphoreCreateBinary();

    xTaskCreate(trigger_task, "Trigger_task", 256, NULL, 1, NULL);
    xTaskCreate(echo_task, "Echo_task", 256, NULL, 1, NULL);
    xSemaphoreGive(xSemaphore_trig);

    xTaskCreate(led_task, "Led_task", 4095, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
