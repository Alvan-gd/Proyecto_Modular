# Wearable Audio Event Detector for Hearing Impaired Assistance

An advanced embedded systems and TinyML project designed to assist hearing-impaired individuals by detecting and classifying relevant environmental sounds. The system processes real-time audio on-edge and provides immediate tactile (vibrational) and visual (luminous) feedback. 

This project encompasses the entire product development lifecycle, from digital signal processing (DSP) and machine learning inference compilation to custom PCB design with strict form-factor constraints.

---

## 1. Project Status & Methodology
This project is currently in the **Research and Architecture Definition** phase. Development follows a V-Model methodology ensuring that requirements tracking, module abstraction, and subsequent verification layers (Unit Testing/HIL) are established prior to hardware freeze.

---

## 2. System Architecture

The architecture is divided into decoupled layers to ensure portability, adherence to hardware abstraction principles, and efficient execution of Digital Signal Processing (DSP) and Machine Learning algorithms.

```mermaid
graph TD
    %% Hardware/Physical Layer
    subgraph Physical_Layer [Physical & Hardware Layer]
        Mic[Digital I2S Microphone] -->|I2S Protocol: SCK, WS, SD| MCU[STM32 Microcontroller]
        MCU -->|GPIO / PWM| Actuators[Vibratory & Luminous Indicators]
    end

    %% Firmware Pipeline
    subgraph Firmware_Pipeline [Firmware DSP & TinyML Pipeline]
        MCU -->|DMA Transfer| PingPong[Double Buffering / Ping-Pong Buffer]
        PingPong -->|Raw PCM Audio| DSP_Stage[CMSIS-DSP: Pre-processing & Feature Extraction]
        DSP_Stage -->|Log-Mel Spectrogram| TinyML_Engine[STM32Cube.AI / Model Zoo Inference]
        TinyML_Engine -->|Classification Output| Logic[Decision & Actuation Logic]
        Logic -->|Driver Activation| Actuators
    end

    style Physical_Layer fill:#f9f9f9,stroke:#333,stroke-width:1px
    style Firmware_Pipeline fill:#f5f7fa,stroke:#0052cc,stroke-width:1px
2.1 Hardware Layer (PCB Design Constraints)Form Factor: Compact, wearable proportions utilizing low-profile SMD/SMT components.Microcontroller: Target STM32 ARM Cortex-M4 MCU (leveraging the FPU for efficient DSP execution).Audio Acquisition: High-SNR MEMS Microphone interface utilizing native I2S bus.Power Management: Battery-operated design (LiPo) with dedicated power-delivery network (PDN), ultra-low-power sleep states management, and integrated charging circuitry.Actuation Drivers: Low-side MOSFET switch configuration for a high-efficiency eccentric rotating mass (ERM) or linear resonant actuator (LRA) vibrator motor, and PWM-controlled LED notification array.2.2 Firmware & Software ArchitectureDrivers & Abstraction: Built on top of STM32Cube HAL y raw CMSIS core registers for deterministic hardware control.Concurrency: Bare-metal scheduling optimized through hardware interrupts (ISRs) and Direct Memory Access (DMA) transfers to prevent CPU starvation during heavy audio streaming.DSP Pipeline: * Audio chunks are collected via a Ping-Pong buffer mechanism (Half-Transfer & Transfer-Complete DMA interrupts).Windowing (Hann/Hamming) and Fast Fourier Transform (FFT) or Mel-Frequency Cepstral Coefficients (MFCC) generation using optimized CMSIS-DSP functions.TinyML Inference Engine: * Model architectures sourced and optimized from the STMicroelectronics Model Zoo.Quantization ($INT8$) and compilation for edge-execution utilizing STM32Cube.AI to minimize Flash and RAM footprint.3. Anticipated Technical RoadmapPhase 1: Research & Architectural Definition (Current)[ ] Finalize selection of the specific STM32 microcontroller and peripheral ICs.[ ] Define acoustic feature extraction pipelines (Sampling frequency, frame length, overlap).[ ] Establish baseline dataset requirements for relevant sound events (e.g., alarms, doorbells, traffic sirens).Phase 2: TinyML & DSP Modeling[ ] Train and quantize the Audio Event Detection model using Python frameworks.[ ] Validate floating-point vs fixed-point implementation performance.[ ] Benchmark execution cycles using STM32Cube.AI tools.Phase 3: Hardware & Firmware Development[ ] Schematic capture and PCB layout optimization for signal integrity (audio traces shield/isolation).[ ] Implement low-level peripheral configuration (I2S, DMA, TIM-PWM, UART for telemetry).[ ] Integrate CMSIS-DSP routines with the TinyML inference library.Phase 4: Validation & Integration[ ] Unit testing execution for core DSP functions.[ ] Hardware-in-the-Loop (HIL) injection testing of raw audio vectors to evaluate accuracy under realistic hardware power profiles.4. Development Tools & StackHardware Design: KiCad / Altium Designer.Firmware IDE: STM32CubeIDE / VS Code with STM32 VS Code Extension.Libraries: STM32Cube MCU Packages (HAL/LL), CMSIS-DSP, CMSIS-NN.Machine Learning Compiler: STM32Cube.AI Core.Telemetry & Scripts: Python 3.x for auxiliary testing and data parsing.
***

###
