# Hardware Design

This directory contains the electronic design of the wearable device, completely isolated from the firmware ecosystem. 

**Toolchain:** The PCB design is developed using **Proteus Design Suite** (ISIS for schematic capture, ARES for PCB layout), chosen for its streamlined workflow and integrated simulation capabilities during the prototyping phase.

**Directory Structure:**
* `/proteus_project`: Contains the main Proteus project files (`.pdsprj`), schematics, and local component libraries.
* `/exports`: PDF exports of the schematic and 3D visualization screenshots.
* `/gerbers`: (Ignored by Git) Fabrication files generated for the PCB manufacturer.