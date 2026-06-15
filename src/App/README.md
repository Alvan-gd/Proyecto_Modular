# Application Logic Layer

This directory contains the core state machine and high-level decision logic of the wearable device. **It is entirely hardware-agnostic.**

**Design Rules:**
1. **Dependency Inversion:** No `.c` file in this directory should directly include `stm32f4xx_hal.h` or manipulate hardware registers.
2. All physical interactions (e.g., triggering the haptic motor, reading battery levels) must be performed through abstract BSP (Board Support Package) wrapper functions.
3. This module acts as the orchestrator: it polls the `Audio_DSP` module for new data, passes it to the `TinyML` engine, and triggers actuators based on the classification confidence.