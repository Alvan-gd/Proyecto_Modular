# Wearable Audio Event Detector for Hearing Impaired Assistance

An advanced embedded systems and TinyML project designed to assist hearing-impaired individuals by detecting and classifying relevant environmental sounds. The system processes real-time audio on-edge and provides immediate tactile (vibrational) and visual (luminous) feedback. 

This project encompasses the entire product development lifecycle, from digital signal processing (DSP) and machine learning inference compilation to custom PCB design with strict form-factor constraints.

---

## 1. Project Status
This project is currently in the **Research and Architecture Definition** phase.

---

## 2. System Architecture

The architecture is divided into decoupled layers to ensure portability, adherence to hardware abstraction principles, and efficient execution of Digital Signal Processing (DSP) and Machine Learning algorithms.

```mermaid
flowchart TD
    %% Definición de las Calles (Swimlanes)
    
    subgraph Entorno Físico
        E1((Sonido Externo))
        E2((Usuario percibe alerta))
    end

    subgraph Hardware y Periféricos
        H1[Micrófonos I2S INMP441]
        H2[Controlador DMA de Hardware]
        H3[Controlador Háptico I2C]
        H4[Pantalla SPI]
    end

    subgraph Core 0 SRAM: Ruta Crítica IA
        C0_1[Llenado de Búfer Circular DMA]
        C0_2{¿Búfer Completo?}
        C0_3[Algoritmo TDOA: Cálculo de Dirección]
        C0_4[DSP: FFT y Espectrograma de Mel]
        C0_5[Inferencia TinyML Multiclase]
        C0_6{¿Confianza > Umbral?}
        C0_7{¿Clase Relevante?}
        C0_8[Path Negativo: Descartar y Limpiar]
        C0_9[Empacar Vector de Tareas y Enviar a Queue]
    end

    subgraph Core 1 PSRAM: Interfaz y Gráficos
        C1_1[Esperar Evento en Cola RTOS]
        C1_2[Extraer Mensaje de Cola]
        C1_3[Mapear Icono desde Flash/PSRAM]
        C1_4[Mapear Comando Háptico]
        C1_5[Transmitir Datos a Buses]
    end

    %% Flujo de la señal
    E1 --> H1
    H1 -- "Audio a 16 kHz" --> H2
    H2 --> C0_1
    C0_1 --> C0_2
    
    %% Camino negativo del buffer
    C0_2 -- "No (Esperar)" --> C0_1
    
    %% Ruta principal
    C0_2 -- "Sí (Muestra lista)" --> C0_3
    C0_3 --> C0_4
    C0_4 --> C0_5
    C0_5 --> C0_6
    
    %% Caminos Negativos de Clasificación
    C0_6 -- "No (Baja Confianza / Ruido)" --> C0_8
    C0_6 -- "Sí" --> C0_7
    C0_7 -- "No (Clase Ignorada)" --> C0_8
    C0_8 -- "Reiniciar Ciclo" --> C0_1
    
    %% Camino Crítico (Emergencia/Aviso)
    C0_7 -- "Sí (Aviso/Urgencia/Emergencia)" --> C0_9
    
    %% Salto entre núcleos mediante FreeRTOS
    C0_9 -- "FreeRTOS Queue (No bloqueante)" --> C1_1
    
    %% Procesamiento del Core 1
    C1_1 --> C1_2
    C1_2 --> C1_3
    C1_2 --> C1_4
    C1_3 --> C1_5
    C1_4 --> C1_5
    
    %% Salida física
    C1_5 -- "Trama I2C Hex" --> H3
    C1_5 -- "Trama SPI (Bitmap)" --> H4
    H3 --> E2
    H4 --> E2
    
    %% Estilos visuales
    classDef hardware fill:#f9f,stroke:#333,stroke-width:2px;
    classDef core0 fill:#bbf,stroke:#333,stroke-width:2px;
    classDef core1 fill:#bfb,stroke:#333,stroke-width:2px;
    
    class H1,H2,H3,H4 hardware;
    class C0_1,C0_2,C0_3,C0_4,C0_5,C0_6,C0_7,C0_8,C0_9 core0;
    class C1_1,C1_2,C1_3,C1_4,C1_5 core1;
```

### 2.1 Hardware Layer (PCB Design Constraints)
* **Form Factor:** Compact, wearable proportions utilizing low-profile SMD/SMT components.
* **Microcontroller:** Target STM32 ARM Cortex-M4 MCU (leveraging the FPU for efficient DSP execution).
* **Audio Acquisition:** High-SNR MEMS Microphone interface utilizing native I2S bus.
* **Power Management:** Battery-operated design (LiPo) with dedicated power-delivery network (PDN), ultra-low-power sleep states management, and integrated charging circuitry.
* **Actuation Drivers:** Low-side MOSFET switch configuration for a high-efficiency eccentric rotating mass (ERM) or linear resonant actuator (LRA) vibrator motor, and PWM-controlled LED notification array.

### 2.2 Firmware & Software Architecture
* **Drivers & Abstraction:** Built on top of STM32Cube HAL and raw CMSIS core registers for deterministic hardware control.
* **Concurrency:** Bare-metal scheduling optimized through hardware interrupts (ISRs) and Direct Memory Access (DMA) transfers to prevent CPU starvation during heavy audio streaming.
* **DSP Pipeline:** * Audio chunks are collected via a Ping-Pong buffer mechanism (Half-Transfer & Transfer-Complete DMA interrupts).
  * Windowing (Hann/Hamming) and Mel-Frequency Cepstral Coefficients (MFCC) generation using optimized CMSIS-DSP functions.
* **TinyML Inference Engine:** * Model architectures sourced and optimized from the STMicroelectronics Model Zoo.
  * Quantization ($INT8$) and compilation for edge-execution utilizing STM32Cube.AI to minimize Flash and RAM footprint.

---

## 3. Anticipated Technical Roadmap

**Phase 1: Research & Architectural Definition (Current)**
- [ ] Finalize selection of the specific ESP32 microcontroller and peripheral ICs.
- [ ] Define acoustic feature extraction pipelines (Sampling frequency, frame length, overlap).
- [ ] Establish baseline dataset requirements for relevant sound events (e.g., alarms, doorbells, traffic sirens).

**Phase 2: TinyML & DSP Modeling**
- [ ] Train and quantize the Audio Event Detection model using Python frameworks.
- [ ] Validate floating-point vs fixed-point implementation performance.
- [ ] Benchmark execution cycles using STM32Cube.AI tools.

**Phase 3: Hardware & Firmware Development**
- [ ] Schematic capture and PCB layout optimization for signal integrity (audio traces shield/isolation).
- [ ] Implement low-level peripheral configuration (I2S, DMA, TIM-PWM, UART for telemetry).
- [ ] Integrate CMSIS-DSP routines with the TinyML inference library.

**Phase 4: Validation & Integration**
- [ ] Unit testing execution for core DSP functions.
- [ ] Hardware-in-the-Loop (HIL) injection testing of raw audio vectors to evaluate accuracy under realistic hardware power profiles.

---

## 4. Development Tools & Stack
* **Build System:** CMake + ARM GNU Toolchain (GCC).
* **Firmware IDE:** Visual Studio Code configured with the ESP-IDF for VS Code extension.
* **Hardware Design:** Proteus Design Suite (ISIS / ARES).
* **Libraries:** Espressif MCU Packages
* **Machine Learning:** Edge Impulse framework
```
