#include "APIServer.h"
#include "SwarmOrchestrator.h"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <general_model_path> <coder_model_path>" << std::endl;
        return 1;
    }

    std::string generalModelPath = argv[1];
    std::string coderModelPath = argv[2];
    
    SwarmOrchestrator swarm;

    std::cout << "[Main] Initializing Heterogeneous Swarm..." << std::endl;

    // --- 1. ARCHITECT ---
    // Now fully language-agnostic
    Agent planner("Architect", generalModelPath, 
        "You are a Software Architect. Analyze the user's request. Output a numbered implementation plan.");

    // --- 2. SENIOR DEV ---
    // Polyglot developer instead of C++-only
    Agent coder("Senior Dev", coderModelPath, 
        "You are an expert Developer capable of writing code in C++, Python, Java, and JavaScript. Follow the plan exactly. Use best practices for the target language.");

    // --- 3. QA LEAD ---
    // Generic language-agnostic code reviewer
    Agent reviewer("QA Engineer", generalModelPath, 
        "You are a QA Lead. Review the code for syntax errors. If valid for the target language, say 'PASS'. Otherwise, list bugs.");

    swarm.registerAgent("planner", planner);
    swarm.registerAgent("coder", coder);
    swarm.registerAgent("reviewer", reviewer);

    APIServer api(&swarm);
    api.start(8080);

    return 0;
}
