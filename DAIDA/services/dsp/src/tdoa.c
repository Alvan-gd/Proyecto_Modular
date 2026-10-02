#include "tdoa.h"

#include <math.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_dsp.h"

#define PCM24_SCALE_FLOAT (1.0f / 8388608.0f)

static const char *TAG = "TDOA"; // Tag usado para imprimir mensajes del log

/* Buffers estáticos alineados a 16-bytes para el hardware SIMD del ESP32-S3 */
__attribute__((aligned(16))) static float complex_mic_a[TDOA_COMPLEX_SIZE];
__attribute__((aligned(16))) static float complex_mic_b[TDOA_COMPLEX_SIZE];
__attribute__((aligned(16))) static float complex_gcc[TDOA_COMPLEX_SIZE];
__attribute__((aligned(16))) static float hann_window[TDOA_FFT_N];

// Bandera de inicialización
static bool is_initialized = false;

/* ###################################################################################
                                FUNCIONES PRIVADAS DSP
    ################################################################################### */


/**
 * @brief Parabolic interpolation to refine the estimation of the time delay of arrival (TDOA) between two signals.
 * This function takes three points from the cross-correlation function (at lags -1, 0, and +1) and fits a parabola to these points to find the vertex, which gives a more accurate estimate of the time delay.
 * 
 * @param y1 Value of the cross-correlation function at lag -1.
 * @param y2 Value of the cross-correlation function at lag 0 (the peak).
 * @param y3 Value of the cross-correlation function at lag +1.
 * @return float The refined estimate of the time delay in samples, which can be a fractional value.
 */
static inline float parabolic_interpolation(float y1, float y2, float y3) 
{
    float denominator = 2.0f * (y1 - 2.0f * y2 + y3); // se construye el denominador de la ecuación de interpolación parabólica
    if (fabsf(denominator) < 1e-6f) { // si el denominador es muy pequeño, se evita la división por cero y se retorna 0.0f fabsf evalua el valor absloluto de un float
        return 0.0f;
    }
    return (y1 - y3) / denominator;
}


/**
 * @brief Compute the time delay of arrival (TDOA) between two microphone signals using the Generalized Cross-Correlation with Phase Transform (GCC-PHAT) method.
 * This function extracts the signals from the interleaved buffer, applies a Hann window, computes the FFT, calculates the cross-correlation, and finds the time delay between the two signals.
 * 
 * @param buffer Pointer to the input buffer containing interleaved microphone samples.
 * @param channel_pos Index of the positive microphone channel in the interleaved buffer.
 * @param channel_neg Index of the negative microphone channel in the interleaved buffer.
 * @return float The estimated time delay in samples between the two microphone signals. This value can be fractional due to interpolation.
 */
static float tdoa_compute_pair_lag(const int32_t *buffer, uint8_t channel_pos, uint8_t channel_neg)
{
    // Implementación de la función para calcular el retardo entre dos señales
    // Esta función debe calcular el retardo entre dos señales utilizando la correlación cruzada
    // y devolver el retardo en muestras.
    esp_err_t err;
    
    /* =============================================================================
     * 1: Extraccion de datos por par de microfonos y conversion a numeros complejos
     * ============================================================================*/

    for (int16_t i = 0; i < TDOA_FFT_N; i++) {
        float sample_a = (float)(buffer[i * I2S_STRIDE + channel_pos] >> 8) * hann_window[i] * PCM24_SCALE_FLOAT; // extraccion de la muestra del micrófono positivo y aplicación de la ventana de Hann
        complex_mic_a[2 * i] = sample_a; //Parte real
        complex_mic_a[2 * i + 1] = 0.0f;     //Parte imaginaria
    
        float sample_b = (float)(buffer[i * I2S_STRIDE + channel_neg] >> 8) * hann_window[i] * PCM24_SCALE_FLOAT; // extraccion de la muestra del micrófono negativo y aplicación de la ventana de Hann
        complex_mic_b[2 * i] =  sample_b; //Parte real
        complex_mic_b[2 * i + 1] = 0.0f;     //Parte imaginaria
    }

    /* =============================================================================
     *              2: Paso al dominio de la frecuencia con FFT
     * ============================================================================*/
    err = dsps_fft2r_fc32(complex_mic_a, TDOA_FFT_N); // FFT de la señal del micrófono positivo

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al calcular la FFT de mic_a: %s", esp_err_to_name(err));
        return 0.0f;
    }

    err = dsps_bit_rev_fc32(complex_mic_a, TDOA_FFT_N); // Reordenar los datos de la FFT de mic_a tras la operacion de inversion de bits

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al reordenar la FFT de mic_a: %s", esp_err_to_name(err));
        return 0.0f;
    }

    err = dsps_fft2r_fc32(complex_mic_b, TDOA_FFT_N); // FFT de la señal del micrófono negativo

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al calcular la FFT de mic_b: %s", esp_err_to_name(err));
        return 0.0f;
    }

    err = dsps_bit_rev_fc32(complex_mic_b, TDOA_FFT_N); // Reordenar los datos de la FFT de mic_b tras la operacion de inversion de bits

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al reordenar la FFT de mic_b: %s", esp_err_to_name(err));
        return 0.0f;
    }

    /* =============================================================================
     * 3: Cálculo correlación cruzada generalizada con normalización de fase (GCC-PHAT)
     * ============================================================================*/
    for (int i = 0; i < TDOA_FFT_N; i++) {

        /* Anular DC (Bin 0) y el derrame de la ventana de Hann / ruido sub-grave (Bins 1 y N-1) */
        if (i == 0 || i == 1 || i == (TDOA_FFT_N - 1)) {
            complex_gcc[2 * i]     = 0.0f;
            complex_gcc[2 * i + 1] = 0.0f;
            continue;
        }

        float real_a = complex_mic_a[2 * i];
        float imag_a = complex_mic_a[2 * i + 1];
        float real_b = complex_mic_b[2 * i];
        float imag_b = complex_mic_b[2 * i + 1];

        // Calcular la conjugada de mic_b y multiplicar por mic_a
        float gcc_real = (real_a * real_b + imag_a * imag_b);
        float gcc_imag = (imag_a * real_b - real_a * imag_b);

        // Normalizar la magnitud para obtener la fase
        float mag_squared = (gcc_real * gcc_real) + (gcc_imag * gcc_imag); // Magnitud al cuadrado

        if (mag_squared < 1e-12f) { // Evitar división por cero
            complex_gcc[2 * i] = 0.0f;
            complex_gcc[2 * i + 1] = 0.0f;
            continue;
        }

        float inv_mgn = 1.0f / sqrtf(mag_squared); // Magnitud inversa
    
        complex_gcc[2 * i] = gcc_real * inv_mgn; // Parte real
        complex_gcc[2 * i + 1] = (gcc_imag * inv_mgn); // Parte imaginaria
        complex_gcc[2 * i + 1] = -complex_gcc[2 * i + 1];
    }

    /* =============================================================================
     * 4: Transformada inversa de Fourier para obtener la señal de correlación cruzada normalizada
     * ============================================================================*/

    err = dsps_fft2r_fc32(complex_gcc, TDOA_FFT_N); // FFT de la señal de correlación cruzada normalizada

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al calcular la FFT de gcc: %s", esp_err_to_name(err));
        return 0.0f;
    }

    err = dsps_bit_rev_fc32(complex_gcc, TDOA_FFT_N); // Reordenar los datos de la FFT de gcc

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al reordenar la FFT de gcc: %s", esp_err_to_name(err));
        return 0.0f;
    }

    /* =============================================================================
     * 5: Encontrar el retardo máximo y refinarlo mediante interpolación parabólica
     * ============================================================================*/

    // Encontrar el índice del máximo en la señal de correlación cruzada
    const int MAX_LAG = 4;
    float max_value = -1e9f;
    int best_tau = 0;

    for (int tau = -MAX_LAG; tau <= MAX_LAG; tau++) {
        int index = (tau < 0) ? (TDOA_FFT_N + tau) : tau; // Asegurarse de que el índice esté dentro del rango
        float current_value = complex_gcc[2 * index]; // Solo se considera la parte real

        if (current_value > max_value) {
            max_value = current_value;
            best_tau = tau;
        }
    }

    // Interpolación parabólica para refinar la estimación del retardo
    int tau_prev = best_tau - 1;
    int tau_next = best_tau + 1;

    int idx_prev = (tau_prev < 0) ? (TDOA_FFT_N + tau_prev) : tau_prev;
    int idx_peak = (best_tau < 0) ? (TDOA_FFT_N + best_tau) : best_tau;
    int idx_next = (tau_next < 0) ? (TDOA_FFT_N + tau_next) : tau_next;

    float y1 = complex_gcc[2 * idx_prev]; // Valor en tau_prev
    float y2 = complex_gcc[2 * idx_peak]; // Valor en best_tau
    float y3 = complex_gcc[2 * idx_next]; // Valor en tau_next

    float delta_tau = parabolic_interpolation(y1, y2, y3); // Refinar el retardo usando interpolación parabólica

    return (float)best_tau + delta_tau; // Retornar el retardo refinado
}

/* ###################################################################################
                              INICIALIZACIÓN
    ################################################################################### */


/**
 * @brief Initialize the TDOA (Time Difference of Arrival) processing module.
 * This function sets up the necessary resources for TDOA processing, including initializing the FFT and creating a Hann window for signal processing.
 * 
 * @return esp_err_t Returns ESP_OK on successful initialization, or an error code if initialization fails.
 */
esp_err_t tdoa_init(void)
{
    esp_err_t err;

    if (is_initialized) {
        ESP_LOGI(TAG, "TDOA ya ha sido inicializado");
        return ESP_OK;
    }

    err = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE); // Inicializar la FFT de 32 bits en punto flotante
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al inicializar la FFT: %s", esp_err_to_name(err));
        return err;
    }

    // Inicializar la ventana de Hann
    dsps_wind_hann_f32(hann_window, TDOA_FFT_N);

    is_initialized = true;
    ESP_LOGI(TAG, "TDOA inicializado correctamente");
    return ESP_OK;

}

/* ###################################################################################
                              PROCESAMIENTO DE TDOA
    ################################################################################### */


/**
 * @brief Process the input buffer to compute the angle of arrival (AoA) using Time Difference of Arrival (TDOA) techniques.
 * This function calculates the time delays between pairs of microphones, computes the angle of arrival based on these delays, and returns the result.
 * 
 * @param buffer Pointer to the input buffer containing interleaved microphone samples.
 * @param angle_out Pointer to a float variable where the computed angle of arrival will be stored.
 * @return esp_err_t Returns ESP_OK on success, ESP_ERR_INVALID_STATE if TDOA is not initialized, ESP_ERR_INVALID_ARG if input pointers are NULL, or ESP_ERR_NOT_FOUND if the signal does not have a clear directional correlation.
 */
esp_err_t tdoa_process(const int32_t *buffer, float *angle_out){

    if (!is_initialized) {
        ESP_LOGE(TAG, "TDOA no ha sido inicializado");
        return ESP_ERR_INVALID_STATE;
    }

    if (buffer == NULL || angle_out == NULL) {
        ESP_LOGE(TAG, "Punteros de buffer o salida nulos");
        return ESP_ERR_INVALID_ARG;
    }

    /* =============================================================================
     *             1: Calculo del retardo entre pares de micrófonos
     * ============================================================================*/

    // Calcular los retardos entre los pares de micrófonos
    float tau_x = tdoa_compute_pair_lag(buffer, TDOA_CH_X_POS, TDOA_CH_X_NEG);
    float tau_y = tdoa_compute_pair_lag(buffer, TDOA_CH_Y_POS, TDOA_CH_Y_NEG);

    /* Si la magnitud del vector (tau_x, tau_y) es menor a 0.2 muestras (al cuadrado = 0.04f),
    * el sonido viene exactamente de arriba/abajo o no hay correlación direccional clara */
    float tau_mag_sq = (tau_x * tau_x) + (tau_y * tau_y);
    if (tau_mag_sq < 0.04f) {
        return ESP_ERR_NOT_FOUND;
    }

    /* =============================================================================
     *                  2: Calculo del ángulo de llegada (AoA)
     * ============================================================================*/
    // En un plano cartesiano, el ángulo se puede calcular usando la función atan2,
    // que devuelve el ángulo en radianes entre el eje x positivo y el punto (tau_x, tau_y)
    // Luego, se convierte a grados multiplicando por (180 / π)
    float angle = atan2f(tau_y, tau_x) * (180.0f / (float)M_PI);

    if (angle < 0.0f) {
        angle += 360.0f; // Asegurarse de que el ángulo esté en el rango [0, 360)
    }

    *angle_out = angle;
    return ESP_OK;
}
