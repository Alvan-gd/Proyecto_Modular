#include "daida_ui.h"
#include "daida_compute.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "DAIDA_UI";

static drv2605_dev_t *haptic_dev = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static lv_display_t *disp = NULL;

// Local queue handle for Core 1 to receive events processed by Core 0
static QueueHandle_t s_ui_queue = NULL;

// Events definitions (sound_type numbers are preeliminary)
static const ui_event_t sismic_alert = {
    .bitmap = alarm_bitmap,
    .urgency_level = HIGH_URGENCY_UI,
    .sound_type = 0
};

static const ui_event_t car_horn = {
    .bitmap = car_bitmap,
    .urgency_level = HIGH_URGENCY_UI,
    .sound_type = 2
};

static const ui_event_t gunshot = {
    .bitmap = gunshot_bitmap,
    .urgency_level = HIGH_URGENCY_UI,
    .sound_type = 3
};

static const ui_event_t dog_bark = {
    .bitmap = dog_bitmap,
    .urgency_level = MEDIUM_URGENCY_UI,
    .sound_type = 4
};

static const ui_event_t user_name = {
    .bitmap = voice_bitmap,
    .urgency_level = SPECIAL_URGENCY_UI,
    .sound_type = 5
};

static const ui_event_t ringbell = {
    .bitmap = bell_bitmap,
    .urgency_level = LOW_URGENCY_UI,
    .sound_type = 6
};
 

/**
 * @brief Main FreeRTOS task assigned to Core 1 to process visual and haptic feedback.
 */
void daida_ui_task(void *pvParameters) {
    daida_compute_event_t received_event;

    ESP_LOGI(TAG, "UI and Control task started on Core 1");

    while (1) {
        // Wait indefinitely until an event packet is received from Core 0
        if (xQueueReceive(s_ui_queue, &received_event, portMAX_DELAY) == pdTRUE) {
            
            // Access information sent from daida_compute
            float detected_angle = received_event.angle;
            uint8_t sound_type = received_event.audio_classification;

            ESP_LOGI(TAG, "Event received on Core 1 -> Angle: %.2f°, Sound Type: %u", detected_angle, sound_type);

            border_zone_t ui_sector = angle_to_sector((int) detected_angle);
            dada_ui_feedback_t feedback = {
                .sector = ui_sector,
                .event = {NULL}
            };
            switch (sound_type){
                case 0:
                    feedback.event = sismic_alert;
                break;
                case 2:
                    feedback.event = car_horn;
                break;
                case 3:
                    feedback.event = gunshot;
                break;
                case 4:
                    feedback.event = dog_bark;
                break;
                case 5:
                    feedback.event = user_name;
                break;
                case 6:
                    feedback.event = ringbell;
                break;
                default:
                screen_clear();
                break;
            }
            draw_indication(feedback.sector, feedback.event.bitmap, feedback.event.urgency_level);
            if (feedback.event.urgency_level == LOW_URGENCY_UI) haptic_play_low_urgency(haptic_dev);
            if (feedback.event.urgency_level == MEDIUM_URGENCY_UI) haptic_play_medium_urgency(haptic_dev);
            if (feedback.event.urgency_level == HIGH_URGENCY_UI) haptic_play_high_urgency(haptic_dev);
            if (feedback.event.urgency_level == SPECIAL_URGENCY_UI) haptic_play_special_urgency(haptic_dev);
        }
    }
}

/**
 * @brief Initializes Core 1 services (Haptics + Screen), creates the UI queue,
 *        links the compute module (Core 0), and launches the UI task on Core 1.
 */
esp_err_t daida_ui_init(QueueHandle_t daida_intercore_queue) {
    ESP_LOGI(TAG, "Initializing Core 1 subsystems (UI and Haptics)...");

    if(daida_intercore_queue == NULL){
        ESP_LOGI(TAG,"Null queue parameter");
        return ESP_ERR_INVALID_ARG;
    }
    
    s_ui_queue = daida_intercore_queue;

    // 1. Initialize the haptic controller
    esp_err_t err = haptic_init(&haptic_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize haptic controller: %s", esp_err_to_name(err));
        return err;
    }

    // 2. Initialize the display and graphics system (LVGL)
    err = screen_init(&io_handle, &panel_handle, &disp);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display: %s", esp_err_to_name(err));
        return err;
    }

    // 4. Create the UI and Control task pinned strictly to Core 1
    BaseType_t task_created = xTaskCreatePinnedToCore(
        daida_ui_task,          // Task function
        "daida_ui_task",        // Task name
        4096,                   // Stack size in bytes
        NULL,                   // Parameters
        5,                      // Priority
        NULL,                   // Task handle
        1                       // Pinned to Core 1
    );

    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create daida_ui_task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "UI subsystems successfully initialized on Core 1");
    return ESP_OK;
}

// Función helper para enviar eventos a la cola desde main o desde otro módulo
QueueHandle_t daida_ui_get_queue_handle(void) {
    return s_ui_queue;
}