# 🚀 Man vs Math: WebAssembly Engine Prototype

**Built for the TapTap Hackathon — Checkpoint 3**

## 🏆 Live Demo & Submission Links
* **🎮 Playable Prototype:** [Play Man vs Math Live Here](https://spectrumsupreme.github.io/TapTap-Spectrum/)


---

![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![WebAssembly](https://img.shields.io/badge/WebAssembly-654FF0?style=for-the-badge&logo=webassembly&logoColor=white)
![SDL2](https://img.shields.io/badge/SDL2-17385E?style=for-the-badge&logo=c&logoColor=white)
![Tailwind CSS](https://img.shields.io/badge/Tailwind_CSS-38B2AC?style=for-the-badge&logo=tailwind-css&logoColor=white)

An interactive, browser-native 2D physics engine built from scratch in C++ and cross-compiled to WebAssembly. This prototype demonstrates a strictly **Data-Driven Architecture**, where all game logic, entity kinematics, and boss AI are controlled entirely via a JSON configuration file, requiring zero C++ recompilation to modify the game state.

## ⚙️ The Architecture

Unlike standard web games built in JS/TS frameworks, this engine runs natively in the browser's memory heap using Emscripten.

* **The Core (Backend):** A custom C++ engine utilizing SDL2 for rendering and Euler integration for custom gravity/velocity physics.
* **The Bridge (WASM):** Compiled into a `.wasm` binary, allowing near-native C++ execution speeds directly in the browser.
* **The Interface (Frontend):** A responsive, dark-mode UI built with Tailwind CSS that hooks directly into the Emscripten JavaScript API to control the C++ runtime lifecycle.

## 🛠️ Data-Driven Design (`config.json`)

Per the hackathon requirements, the engine is fully modular. By modifying the `config.json` file, developers can instantly alter:
* **Entity Kinematics:** Player gravity, spawn coordinates, and movement speed.
* **Boss AI:** Switch the state machine from `'bounce'` to `'chase'`, or alter its Phase 2 mechanics (e.g., `'earthquake'`).
* **Weapon Systems:** Toggle the player's weapon class between `'charge_shot'` and `'auto_rifle'`.

## 💻 Building from Source (Windows PowerShell)

If you are pulling this repository to modify the C++ engine or the `config.json` file, you must recompile the WebAssembly package using Emscripten.

Run the following commands in your Windows PowerShell terminal at the project root:

**1. Bypass Windows Execution Policy (if restricted):**
~~~powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
~~~

**2. Activate the Emscripten SDK:**
~~~powershell
.\emsdk_env.ps1
~~~

**3. Compile the Engine and Package the Virtual File System:**
*(Note: We output to `index.js` to preserve the custom Tailwind `index.html` frontend).*
~~~bash
emcc engine.cpp -o index.js -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' --preload-file images --preload-file config.json -s ALLOW_MEMORY_GROWTH=1
~~~

## 🚀 How to Run Locally

To test the compiled build locally:
1. Ensure you have Python installed.
2. Open a terminal in the root directory.
3. Run a local server: `python -m http.server 8080`
4. Open a browser (Incognito recommended to avoid WASM caching) and navigate to `http://localhost:8080`.

## 🎮 Game Rules

1. **Movement:** `A` / `D` to move, `Space` to jump. 
2. **Combat:** Left Click to fire. Holding the mouse button charges your weapon for massive damage and projectile scaling.
3. **I-Frames:** Taking damage grants 1 second of invincibility.
4. **Phase 2:** When the boss hits 50% HP, watch the floor. If it flashes Orange, prepare to jump to avoid the Red earthquake attack!

## 👥 The Team

* **Dhiraj Adhikary** — *Lead Systems Engineer*
  * Designed the C++ Engine, AABB Collision Physics, JSON Serialization, and the WebAssembly/Emscripten pipeline.
* **Samyak** — *Frontend Architect*
  * Developed the responsive Tailwind UI, designed the WebGL canvas wrapper, and implemented the JS-to-WASM memory bridge.

---
