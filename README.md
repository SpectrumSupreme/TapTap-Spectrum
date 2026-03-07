# TaPTaP Game Engine - Blueprint

**TaPTaP Game Engine Hackathon 2026 - League 1 Submission**

## 📌 Mission Statement

We are designing and building a reusable, web-based learning game engine capable of generating multiple playable learning experiences entirely through configuration. This engine prioritizes system architecture over a single prototype, focusing on scalability, extensibility, and the strict separation of logic and content.

## 👥 Team

- **Dhiraj Adhikary** - Role: Core Development (Specifics TBD)
- **Samyak Anil Gaikwad** - Role: Core Development (Specifics TBD)
  _(Note: Both members are from IIT Guwahati, fulfilling the same-institute requirement)_

## 🏗️ Core System Architecture

Our engine treats game execution as a dynamic state machine. By abstracting the game loop away from the visual assets, we ensure maximum modularity The system runs purely on web technologies and is divided into three distinct layers:

1. **The Data Layer (Configuration):** The JSON file acts as the sole source of truth. It defines the initial state, scoring rules, level structures, and asset paths. This ensures absolute separation of logic and content.
2. **The Core Engine (State Manager / Controller):** The algorithmic heart of the engine. A parser reads the JSON input and maps it into memory. The Game Loop runs continuously, processing rules against the current state, handling inputs, and determining the next state independently of the visual output.
3. **The Presentation Layer (Web Renderer):** A lightweight module built on web technologies. It strictly accepts the calculated "current state" from the Core Engine and draws it to the screen, allowing the renderer to be upgraded or swapped without breaking the core logic.

## ⚙️ JSON-Driven Configuration Schema

The engine is driven entirely by JSON configuration , allowing it to support multiple games from a single engine. Below is our baseline schema demonstrating how a new learning game (e.g., a logic sequence puzzle) can be generated without altering the underlying engine code:

```json
{
  "game_metadata": {
    "title": "Spectrum Pattern",
    "learning_objective": "Pattern Recognition and Sequencing",
    "win_condition": "current_level > total_levels"
  },
  "mechanics": {
    "type": "sequence_puzzle",
    "input_method": "grid_click",
    "max_mistakes_allowed": 3
  },
  "global_assets": {
    "lock_sound": "/assets/audio/click.mp3",
    "error_sound": "/assets/audio/buzzer.mp3"
  },
  "levels": [
    {
      "level_id": "1",
      "puzzle_prompt": "Input the sequence of prime numbers.",
      "grid_options": [1, 2, 3, 4, 5, 6, 7, 8, 9],
      "target_sequence": [2, 3, 5, 7],
      "on_success": {
        "action": "unlock_animation",
        "next_state": "level_2"
      },
      "on_fail": {
        "action": "register_mistake",
        "retry": true
      }
    }
  ]
}
```
