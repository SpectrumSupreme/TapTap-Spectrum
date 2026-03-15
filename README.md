# TaPTaP Engine - Core Proof
**Team:** Dhiraj & Samyak

## Overview
This repository contains the Checkpoint 2 Core Engine Proof for the TaPTaP Hackathon. It demonstrates a working, event-driven engine loop that is entirely data-driven via JSON. 

## Engine/Data Separation Boundary
Our architecture enforces a strict boundary between game logic and game data:
* **The Logic (The Engine):** `parser.cpp` acts as a general-purpose state machine. It handles input evaluation, mistake tracking, and level progression, but it contains zero hardcoded game knowledge.
* **The Data (The Content):** All game parameters (prompts, targets, rules) are injected at runtime via JSON files (`schema1.json` and `schema2.json`).

Because of this modularity, swapping the JSON file produces a completely different game using the exact same compiled engine executable.

## JSON Schema Documentation
The engine expects a JSON file with the following structural schema:
* `game_metadata`: Contains the `title` of the game.
* `mechanics`: Defines the `max_mistakes_allowed` (integer).
* `levels`: An array of objects, where each level contains:
    * `level_id`: Unique identifier (string).
    * `puzzle_prompt`: The text displayed to the user (string).
    * `target_sequence`: The required answers to pass the level (array of integers).

## How to Run Locally
The engine currently runs in a terminal environment to cleanly demonstrate the data pipeline before we hook up the Emscripten WebAssembly bridge for the web UI.

**Prerequisites:**
* A standard C++ compiler (e.g., GCC/G++).
* Ensure `json.hpp` is in the same directory as `parser.cpp`.

**Execution Steps:**
1. Clone the repository and navigate to the folder.
2. Compile the engine:
   ```bash
   g++ parser.cpp -o engine
