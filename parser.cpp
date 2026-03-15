#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "json.hpp"
using namespace std;
using json = nlohmann::json;
struct Level {
    string level_id;
    string puzzle_prompt;
    vector<int> target_sequence;
};
struct GameConfig {
    string title;
    int max_mistakes;
    vector<Level> levels;
};
GameConfig config;
int current_level_idx = 0;
int mistakes_made = 0;
int current_sequence_idx = 0; 
void boot_engine(string filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open " << filename << endl;
        exit(1);
    }
    json gameData;
    file >> gameData;

    config.title = gameData["game_metadata"]["title"];
    config.max_mistakes = gameData["mechanics"]["max_mistakes_allowed"];
    for (auto& lvl : gameData["levels"]) {
        Level newLevel;
        newLevel.level_id = lvl["level_id"];
        newLevel.puzzle_prompt = lvl["puzzle_prompt"];
        newLevel.target_sequence = lvl["target_sequence"].get<vector<int>>();
        config.levels.push_back(newLevel);
    }
    cout << "\n=== Example Game: " << config.title << " ===" << endl;
    cout << "[Level " << config.levels[current_level_idx].level_id << "] " 
         << config.levels[current_level_idx].puzzle_prompt << endl;
}
int submit_guess(int guess) {
    Level& currentLevel = config.levels[current_level_idx];
    int expected_val = currentLevel.target_sequence[current_sequence_idx];

    if (guess == expected_val) {
        cout << "Correct! (" << guess << ")" << endl;
        current_sequence_idx++;
        if (current_sequence_idx >= currentLevel.target_sequence.size()) {
            cout << "-> SUCCESS! Level " << currentLevel.level_id << " cleared.\n";
            current_level_idx++;
            current_sequence_idx = 0; 
            if (current_level_idx < config.levels.size()) {
                cout << "\n[Level " << config.levels[current_level_idx].level_id << "] " 
                     << config.levels[current_level_idx].puzzle_prompt << endl;
            } else {
                cout << "\nSYSTEM UNLOCKED. You Win!" << endl;
                return 2; 
            }
        }
        return 1; 
    } else {
        mistakes_made++;
        cout << "-> ERROR! Incorrect. Mistakes: " << mistakes_made << "/" << config.max_mistakes << "\n";
        current_sequence_idx = 0; 
        if (mistakes_made >= config.max_mistakes) {
            cout << "\nMAX MISTAKES REACHED. Game Over." << endl;
            return -1;
        }
        return 0; 
    }
}
int main() {
    string file_to_load;
    cout << "Enter schema to load (e.g., schema1.json or schema2.json): ";
    cin >> file_to_load;
    boot_engine(file_to_load);      
    int guess;
    while (true) {
        cout << "> Enter your guess: ";
        if (!(cin >> guess)) break;        
        int status = submit_guess(guess);
        if (status == 2 || status == -1) {
            break;
        }
    }
    return 0;
}
