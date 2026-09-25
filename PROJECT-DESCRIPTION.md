# Retro Garage Workshop

112/2021 - Danilo Nikolas
A retro garage workshop containing a mechanic welder, a workbench, and an engine being repaired. The application features high-quality real-time lighting with Blinn-Phong shading, point shadows (with PCF), post-processing effects (HDR tone mapping and Bloom), and custom loader enhancements for recursive node transformations and PBR colors.

## Controls

* `W, A, S, D` -> Move camera  
* `Mouse Drag / Movement` -> Look around  
* `Scroll wheel` -> Zoom camera  
* `F1` -> Toggle cursor lock (for camera rotation vs. UI interaction)  
* `F2` -> Toggle ImGui control panel  
* `L` -> Start welding / repairing the engine  
* `R` -> Reset welding and engine repair state  

## Features

### Fundamental:

* [x] Model with lighting
* [x] Three types of lighting with customizable properties through the GUI (Directional light / moonlight + Point light / welding spark + Spot light / ceiling neon lamp)
* [x] `L key (Start Repair)` --- AFTER 3 SECONDS --- Triggers ---> `Welder heats up and glows RED` --- AFTER 2 SECONDS --- Triggers ---> `Engine is repaired (broken engine model swapped to repaired engine model)`

### Group A:

* [x] Bloom with the use of HDR (Multi-pass Gaussian Blur and Tone Mapping)

### Group B:

* [x] Point Shadows (Dual shadow casting mapping for welder and garage light)

### Engine improvement:

* []

## Models:

* [Garage Room Model](https://sketchfab.com/3d-models/garage-render-redone-free-870bc832bfd64abea5a4e91a44b641a4)
* [Welder Model (009 Male Worker Welder 02)](https://sketchfab.com/3d-models/009-male-worker-welder-02-b6aa2d7b550d4b3c8a7df5c3b414e5e9)
* [Ceiling Lamp Model](https://sketchfab.com/3d-models/ceiling-lamp-9289f2d239c3469c828def473635db99)
* [Broken Engine Model (Car Engine 19MB)](https://sketchfab.com/3d-models/car-engine-19-mb-2c4ddc62fe5f43bca2fe363ed26db087)
* [Repaired Engine Model (F6 Boxer Engine)](https://sketchfab.com/3d-models/f6-boxer-engine-5700259eeb494b8f8a0b8f63486c59cc)

## Textures:

* [Skybox Textures](https://freestylized.com/skybox/sky_87/)
* Model Textures (Metal, Rust, Concrete, Fabric, etc.) - Integrated directly within the 3D models downloaded from Sketchfab (links above)
