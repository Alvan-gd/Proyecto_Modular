# Digital Signal Processing (DSP) Pipeline

This is a hard real-time module responsible for audio acquisition and feature extraction prior to neural network inference.

**Responsibilities:**
* **DMA Management:** Handling the Ping-Pong buffer callbacks (Half-Transfer & Transfer-Complete) from the I2S microphone to prevent CPU starvation.
* **CMSIS-DSP Execution:** Applying windowing functions (e.g., Hann) and calculating the Log-Mel Spectrogram arrays.

**Memory Warning:**
The static arrays allocated here (PCM buffers and spectrogram outputs) represent the largest RAM consumption in the system. Ensure strict 32-bit word alignment for optimal memory access speeds.