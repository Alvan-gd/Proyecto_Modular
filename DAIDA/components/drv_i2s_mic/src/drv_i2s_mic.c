#include "drv_i2s_mic.h"
#include "inmp441.h"         /* Constantes privadas del silicio del modulo INMP441 */

#include <stdint.h>
#include <stddef.h>
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "soc/gpio_sig_map.h"
#include "esp_rom_gpio.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_periph.h"

static const char *TAG = "DRV_I2S_MIC"; // Tag usado para imprimir mensajes del log

/* Definición de pines para el XIAO ESP32-S3 */
/* Para el uso de una matriz en cruz de microfonos es necesario 
    configurar un canal (controlador) i2s como maestro, el cual
    generará las señales BCLK y WS, mientras que el segundo canal
    será configurado como esclavo y leerá estas mismas señales
    Din0 corresponde a Mic 1/2
    Din1 corresponde a Mic 3/4 */

#define I2S_GPIO_BCLK    GPIO_NUM_1  /* Reloj compartido */
#define I2S_GPIO_WS      GPIO_NUM_2  /* Word Select compartido (L/R) */
#define I2S_GPIO_DIN_0   GPIO_NUM_3  /* Datos Micrófonos 1 y 2 */
#define I2S_GPIO_DIN_1   GPIO_NUM_4 /* Datos Micrófonos 3 y 4 */

static i2s_chan_handle_t i2s_rx_chan_0 = NULL; /* Canal I2S 0 (Maestro) */
static i2s_chan_handle_t i2s_rx_chan_1 = NULL; /* Canal I2S 1 (Esclavo) */

/* Buffers estáticos internos (SRAM) para extraer los datos DMA crudos
512 frames * 2 canales (estéreo) * 4 bytes = 4096 bytes por controlador */
static int32_t raw_dma_buf_0[I2S_DMA_FRAME_NUM * 2];
static int32_t raw_dma_buf_1[I2S_DMA_FRAME_NUM * 2];

/* ###################################################################################
                              INICIALIZACIÓN
    ################################################################################### */


/**
 * @brief Initialize the I2S channels for the INMP441 microphones.
 *  This function configures two I2S channels: one as master and the other as slave, to read audio data from the INMP441 microphones.
 * 
 * @return esp_err_t Returns ESP_OK on success, or an error code if initialization fails.
 */
esp_err_t drv_i2s_mic_init(void){

    esp_err_t err;

    if (i2s_rx_chan_0 != NULL || i2s_rx_chan_1 != NULL) {
        ESP_LOGE(TAG, "Los canales I2S ya han sido inicializados");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Inicializando canales I2S_0 e I2S_1 para micrófonos INMP441...");

    /* =============================================================================
     *                      1: Configuración de los canales I2S
     * ============================================================================*/

    // Configuración del canal I2S 0 (Maestro)
    i2s_chan_config_t i2s_chan_cfg_0 = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_chan_cfg_0.dma_desc_num = 4; /* Número de descriptores DMA */
    i2s_chan_cfg_0.dma_frame_num = I2S_DMA_FRAME_NUM; /* Número de tramas por descriptor */
    i2s_chan_cfg_0.auto_clear = true; /* Limpiar automáticamente el buffer después de la devolución de llamada */

    err = i2s_new_channel(&i2s_chan_cfg_0, NULL, &i2s_rx_chan_0); // se configura el canal como recepción (RX) y se obtiene el handle del canal I2S 0

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al crear el canal I2S 0: %s", esp_err_to_name(err));
        return err;
    }

    // Configuración del canal I2S 1 (Esclavo)
    i2s_chan_config_t i2s_chan_cfg_1 = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_SLAVE);
    i2s_chan_cfg_1.dma_desc_num = 4; /* Número de descriptores DMA */
    i2s_chan_cfg_1.dma_frame_num = I2S_DMA_FRAME_NUM; /* Número de tramas por descriptor */
    i2s_chan_cfg_1.auto_clear = true; /* Limpiar automáticamente el buffer después de la devolución de llamada */

    err = i2s_new_channel(&i2s_chan_cfg_1, NULL, &i2s_rx_chan_1); // se configura el canal como recepción (RX) y se obtiene el handle del canal I2S 1

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al crear el canal I2S 1: %s", esp_err_to_name(err));
        return err;
    }

    /* =============================================================================
     *                  2: Configuración del modo de Operacion (STD)
     * ============================================================================*/

    i2s_std_config_t i2s_std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_SAMPLE_RATE_HZ),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_GPIO_BCLK,
            .ws = I2S_GPIO_WS,
            .dout = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    //i2s_std_cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT; // Configuración de 32 bits por slot para los micrófonos INMP441

    // Aplicar configuracion en modo stadar al canal I2S 1 (Esclavo)
    i2s_std_cfg.gpio_cfg.din = I2S_GPIO_DIN_1; // Configuración de pin de datos para el canal I2S 1

    err = i2s_channel_init_std_mode(i2s_rx_chan_1, &i2s_std_cfg);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al inicializar el canal I2S 1 en modo STD: %s", esp_err_to_name(err));
        return err;
    }

    // Aplicar configuracion en modo stadar al canal I2S 0 (Maestro)
    i2s_std_cfg.gpio_cfg.din = I2S_GPIO_DIN_0; // Configuración de pin de datos para el canal I2S 0

    err = i2s_channel_init_std_mode(i2s_rx_chan_0, &i2s_std_cfg);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al inicializar el canal I2S 0 en modo STD: %s", esp_err_to_name(err));
        return err;
    }

    // --- CIRUGÍA DE SILICIO ---
    // 3. Encendemos el buffer de entrada de los pines físicos a nivel de registro 
    // sin alterar la salida del Maestro.
    PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[I2S_GPIO_BCLK]);
    PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[I2S_GPIO_WS]);

    // 4. Enrutamos la señal de esos pines hacia el periférico Esclavo (I2S_1)
    esp_rom_gpio_connect_in_signal(I2S_GPIO_BCLK, I2S1I_BCK_IN_IDX, false);
    esp_rom_gpio_connect_in_signal(I2S_GPIO_WS,   I2S1I_WS_IN_IDX,  false);


    /* =============================================================================
     *                  3: Arranque de los canales I2S
     * ============================================================================*/

    err = i2s_channel_enable(i2s_rx_chan_1); // Arranque del canal I2S 1 (Esclavo) primero para que esté listo para recibir datos

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al habilitar el canal I2S 1: %s", esp_err_to_name(err));
        return err;
    }

    err = i2s_channel_enable(i2s_rx_chan_0); // Arranque del canal I2S 0 (Maestro)

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al habilitar el canal I2S 0: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Canales I2S inicializados correctamente para micrófonos INMP441.");
    return ESP_OK;
}


/* ###################################################################################
                                    LECTURA
    ################################################################################### */


/**
 * @brief Read audio data from the I2S channels for the INMP441 microphones.
 * 
 * @param buffer_out Pointer to the output buffer where the read audio data will be stored.
 * @param buffer_size Size of the output buffer in bytes. Must be large enough to hold the data from both channels.
 * @param bytes_read Pointer to a variable where the number of bytes read will be stored. Can be NULL if not needed.
 * @param timeout_ms Timeout in milliseconds for the read operation. If the read operation does not complete within this time, it will return an error.
 * 
 * @return esp_err_t Returns ESP_OK on success, or an error code if the read operation fails or if the channels are not initialized.
 */
esp_err_t drv_i2s_mic_read(int32_t *buffer_out, size_t buffer_size, size_t *bytes_read, uint32_t timeout_ms) {

    if (i2s_rx_chan_0 == NULL || i2s_rx_chan_1 == NULL) {
        ESP_LOGE(TAG, "Los canales I2S no han sido inicializados");
        return ESP_ERR_INVALID_STATE;
    }

    if (buffer_out == NULL) {
        ESP_LOGE(TAG, "El puntero de salida del buffer es nulo");
        return ESP_ERR_INVALID_ARG;
    }

    size_t bytes_read_0 = 0;
    size_t bytes_read_1 = 0;

    size_t bytes_per_channel = buffer_size / 2;

    if (bytes_per_channel > sizeof(raw_dma_buf_0)) {
        ESP_LOGE(TAG, "El tamaño del buffer es demasiado grande para la lectura");
        return ESP_ERR_INVALID_ARG;
    }

    /* =============================================================================
     *                   1: Lectura de datos de ambos canales I2S
     * ============================================================================*/

    esp_err_t err;
    int32_t mic_1 = 0, mic_2 = 0, mic_3 = 0, mic_4 = 0;

    // Leer datos del canal I2S 0 (Maestro)
    err = i2s_channel_read(i2s_rx_chan_0, raw_dma_buf_0, bytes_per_channel, &bytes_read_0, timeout_ms);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al leer del canal I2S 0: %s", esp_err_to_name(err));
        return err;
    }

    // Leer datos del canal I2S 1 (Esclavo)
    err = i2s_channel_read(i2s_rx_chan_1, raw_dma_buf_1, bytes_per_channel, &bytes_read_1, timeout_ms);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al leer del canal I2S 1: %s", esp_err_to_name(err));
        return err;
    }

    // Validar que ambos canales hayan leído la misma cantidad de bytes
    if (bytes_read_0 != bytes_read_1) {
        ESP_LOGE(TAG, "Desajuste en la cantidad de bytes leídos: Canal 0 = %d, Canal 1 = %d", bytes_read_0, bytes_read_1);
        return ESP_ERR_INVALID_SIZE;
    }

    /* =============================================================================
     *                   2: Acondicionamiento y Empaquetado DSP
     * ============================================================================*/

    size_t frames_read = bytes_read_0 / (sizeof(int32_t) * 2); // Número de frames leídos por canal

    for (size_t i = 0; i < frames_read; i++) {
       // Extraer muestras de 24 bits de cada canal y almacenarlas en el buffer de salida
       mic_1 = raw_dma_buf_0[2 * i];     // Micrófono 1 (Canal I2S 0, Slot Izquierdo)
       mic_2 = raw_dma_buf_0[2 * i + 1]; // Micrófono 2 (Canal I2S 0, Slot Derecho)
       mic_3 = raw_dma_buf_1[2 * i];     // Micrófono 3 (Canal I2S 1, Slot Izquierdo)
       mic_4 = raw_dma_buf_1[2 * i + 1]; // Micrófono 4 (Canal I2S 1, Slot Derecho)

       // Almacenar las muestras en el buffer de salida en el orden deseado
       buffer_out [4 * i]     = mic_1; // Micrófono 1
       buffer_out [4 * i + 1] = mic_2; // Micrófono 2
       buffer_out [4 * i + 2] = mic_3; // Micrófono 3
       buffer_out [4 * i + 3] = mic_4; // Micrófono 4
    }

    if (bytes_read != NULL) {
       *bytes_read = bytes_read_0 + bytes_read_1; // Retornar la cantidad de bytes leídos
    }

    return ESP_OK; // Return success if all operations complete correctly
}