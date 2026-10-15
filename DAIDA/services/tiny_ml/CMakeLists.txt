file(GLOB_RECURSE SOURCES
    "*.cpp"
    "*.c"
    "*.cc"
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS 
        "."
        "include"
        "edge-impulse-sdk"
        "edge-impulse-sdk/classifier"
        "edge-impulse-sdk/porting"
        "edge-impulse-sdk/porting/espressif/esp-dsp/modules/common/include"
        "edge-impulse-sdk/porting/espressif/esp-dsp/modules/fft/include"
        "edge-impulse-sdk/dsp"
        "edge-impulse-sdk/dsp/kissfft"
        "edge-impulse-sdk/dsp/dct"
        "edge-impulse-sdk/third_party/flatbuffers/include"
        "edge-impulse-sdk/third_party/gemmlowp"
        "edge-impulse-sdk/third_party/ruy"
        "model-parameters"
        "tflite-model"
    PRIV_REQUIRES esp_timer freertos log cxx
)

# SE AÑADIERON BANDERAS PARA SUPRIMIR ERRORES POR WARNINGS DE TENSORFLOW LITE
target_compile_options(${COMPONENT_LIB} PRIVATE 
    -O3 
    -Wno-strict-aliasing 
    -Wno-error 
    -Wno-missing-field-initializers 
    -Wno-deprecated-declarations
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE EI_CLASSIFIER_ALLOCATION_METHOD=1)