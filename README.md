# DAIDA Dispositivo de Asistencia Inteligente para Discapacidad Auditiva

![Status: Architecture & Research](https://img.shields.io/badge/Status-Architecture_&_Research-blue)
![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-orange)
![Framework: ESP-IDF](https://img.shields.io/badge/Framework-ESP--IDF-red)
![RTOS: FreeRTOS](https://img.shields.io/badge/RTOS-FreeRTOS-green)

Un proyecto avanzado de sistemas embebidos y TinyML diseñado para asistir a personas con discapacidad auditiva mediante la detección, cálculo espacial y clasificación de sonidos ambientales críticos. El sistema procesa el audio en tiempo real directamente en el dispositivo (*Edge AI*) y proporciona retroalimentación multisensorial inmediata.

Este proyecto abarca el ciclo de vida completo de desarrollo bajo la metodología del **Modelo V** (estándares INCOSE y VDI 2206), desde el Procesamiento Digital de Señales (DSP) e inferencia estocástica, hasta el diseño de un PCB a medida con estrictas restricciones físicas (latencia <= 150 ms, masa <= 300 g, límite de 11 pines GPIO).

---

## ✨ Características Principales
* **Clasificación Jerárquica de Audio:** Detección de ~20 clases de sonidos ambientales divididas en tres niveles paramétricos (*Aviso, Urgencia, Emergencia*).
* **Resolución Espacial (TDOA):** Cálculo de Dirección de Llegada del sonido con una resolución de 45° (8 cuadrantes).
* **Feedback Multisensorial:** 
  * **Háptico:** Patrones de vibración segregados vía bus I2C.
  * **Visual:** Pantalla SPI con íconos dinámicos y codificación cromática perimetral virtualizada.
* **Procesamiento de Baja Latencia:** Muestreo a 16 kHz y ejecución concurrente para garantizar una respuesta End-to-End <= 150 ms.

---

## 🏗️ Arquitectura del Sistema

La arquitectura implementa un enfoque de multiprocesamiento asimétrico (Dual Core) bajo FreeRTOS, segregando de forma estricta los mapas de memoria para garantizar el determinismo temporal en la ruta crítica del modelo de Inteligencia Artificial.

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

## 🛠️ Stack Tecnológico y Hardware

### Hardware (Dominio Físico)
* **SoC:** Seeed Studio XIAO ESP32-S3 (Xtensa Dual-Core LX7 a 240 MHz, 8MB Flash, 8MB PSRAM).
* **Captura Acústica:** 4x Micrófonos INMP441 (Bus I2S Maestro-Esclavo).
* **Actuador Háptico:** Driver DRV2605L (Bus I2C).
* **Interfaz Visual:** Pantalla TFT/OLED (Bus SPI).

### Software (Dominio Lógico y MLOps)
* **RTOS & Framework:** FreeRTOS sobre ESP-IDF (C/C++).
* **Machine Learning:** Edge Impulse (Exportación optimizada para instrucciones vectoriales Xtensa LX7).
* **Entorno de Desarrollo:** Visual Studio Code + Docker (Contenedor inmutable para compilación cruzada).
* **Diseño EDA:** Proteus Design Suite (Ruteo de Carrier Board PCB).

---

## 📈 Estado Actual (Roadmap)
- [x] Definición de Requerimientos del Sistema (REQ-01 a REQ-08).
- [x] Diseño de Arquitectura (Hardware, Memoria y RTOS).
- [ ] Diseño Detallado y Pruebas de Integración (TDOA y DSP).
- [ ] Implementación de Firmware y Cuantización del Modelo.
- [ ] Fabricación de PCB y Validación de Usuario.
