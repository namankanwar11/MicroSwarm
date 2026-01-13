#pragma once
#include <string>

struct Agent {
    std::string name;           // e.g., "Planner", "Coder"
    std::string modelPath;      // Path to specific .gguf
    std::string systemPrompt;   // The "Personality"
    
    // Constructor for easy creation
    Agent(std::string n, std::string path, std::string prompt) 
        : name(n), modelPath(path), systemPrompt(prompt) {}
};