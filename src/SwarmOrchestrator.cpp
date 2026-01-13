#include "SwarmOrchestrator.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <filesystem> 
#include <fstream>
#include <array>
#include <memory>
#include <stdexcept>
#include <cstdio>
#include <algorithm>
#include <chrono>
#include <thread>  // <--- Add this
#include <chrono>  // <--- Add this
namespace fs = std::filesystem;

SwarmOrchestrator::SwarmOrchestrator() {
    std::cout << "[Orchestrator] System initialized." << std::endl;
}

void SwarmOrchestrator::registerAgent(const std::string& id, const Agent& agent) {
    agents.insert({id, agent});
    std::cout << "[Orchestrator] Registered Agent: " << agent.name << std::endl;
}

std::string SwarmOrchestrator::loadKnowledgeBase() {
    std::string context = "";
    std::string path = "../knowledge"; 

    if (!fs::exists(path)) {
        path = "knowledge"; 
        if (!fs::exists(path)) return "";
    }

    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.path().extension() == ".txt") {
                std::ifstream f(entry.path());
                if (f.is_open()) {
                    std::stringstream buffer;
                    buffer << f.rdbuf();
                    context += "\n--- CONTEXT FROM " + entry.path().filename().string() + " ---\n";
                    context += buffer.str() + "\n";
                }
            }
        }
    } catch (...) {}
    
    if (!context.empty()) {
        std::cout << "[RAG] Injected " << context.length() << " chars of context." << std::endl;
    }
    return context;
}

std::string cleanMarkdown(const std::string& content) {
    std::string cleanCode = content;
    size_t start = cleanCode.find("```");
    if (start != std::string::npos) {
        size_t end = cleanCode.find("\n", start);
        cleanCode = cleanCode.substr(end + 1); 
        size_t last = cleanCode.rfind("```");
        if (last != std::string::npos) {
            cleanCode = cleanCode.substr(0, last); 
        }
    }
    return cleanCode;
}

std::string SwarmOrchestrator::saveArtifact(const std::string& content, const std::string& taskName, const std::string& lang) {
    std::string outDir = "../output";
    if (!fs::exists(outDir)) fs::create_directory(outDir);

    std::string ext = ".txt";
    if (lang == "Python") ext = ".py";
    else if (lang == "C++") ext = ".cpp";
    else if (lang == "JavaScript") ext = ".js";
    else if (lang == "Java") ext = ".java";

    std::string cleanCode = cleanMarkdown(content);

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << outDir << "/Run_" << std::put_time(std::localtime(&now_c), "%H%M%S") << ext;
    std::string fullPath = fs::absolute(ss.str()).string();

    std::ofstream outFile(fullPath);
    if (outFile.is_open()) {
        outFile << cleanCode;
        outFile.close();
        std::cout << "[Artifact] Saved to " << fullPath << std::endl;
        return fullPath; 
    }
    return "";
}

std::string SwarmOrchestrator::executeScript(const std::string& filepath, const std::string& lang) {
    if (filepath.empty()) return "Error: File not found.";
    if (lang != "Python") return "Execution skipped (Only Python supported for sandbox).";

    std::string cmd = "python \"" + filepath + "\" 2>&1";
    std::array<char, 128> buffer;
    std::string result;
    
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
    if (!pipe) return "Error: popen failed!";

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string SwarmOrchestrator::performWebSearch(const std::string& query) {
    std::string toolScript = "../tools/search.py";
    if (!fs::exists(toolScript)) toolScript = "tools/search.py";

    std::string cmd = "python \"" + toolScript + "\" \"" + query + "\"";
    std::array<char, 128> buffer;
    std::string result;

    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
    if (!pipe) return "Error: Search tool failed to launch.";

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string SwarmOrchestrator::runAgent(const std::string& agentId, const std::string& input) {
    if (agents.find(agentId) == agents.end()) return "Error: Agent not found.";

    Agent& agent = agents.at(agentId);

    if (currentLoadedModel != agent.modelPath) {
        engine.loadModel(agent.modelPath);
        currentLoadedModel = agent.modelPath;
    }

    std::stringstream prompt;
    prompt << "<|system|>\n" << agent.systemPrompt << "<|end|>\n";
    prompt << "<|user|>\n" << input << "<|end|>\n";
    prompt << "<|assistant|>\n";

    return engine.query(prompt.str(), 1024);
}

SwarmResult SwarmOrchestrator::runIterativeTask(const std::string& rawInput) {
    std::string targetLang = "C++"; 
    std::string userTask = rawInput;
    
    size_t delimiterPos = rawInput.find("|");
    if (delimiterPos != std::string::npos) {
        targetLang = rawInput.substr(0, delimiterPos);
        userTask = rawInput.substr(delimiterPos + 1);
    }

    std::cout << "\n=== STARTING SELF-HEALING SWARM [" << targetLang << "] ===\n" << std::endl;
    
    // LIGHT 1: ARCHITECT (PLANNING)
    currentStage = 1; 

    double totalSwitchTime = 0;
    double totalGenTime = 0;
    int totalTokens = 0;

    std::string context = loadKnowledgeBase();

    // ----------- WEB SEARCH TRIGGER -----------
    std::string loweredTask = userTask;
    std::transform(loweredTask.begin(), loweredTask.end(), loweredTask.begin(), ::tolower);

    bool needsSearch = (loweredTask.find("search") != std::string::npos || 
                        loweredTask.find("find") != std::string::npos ||
                        loweredTask.find("latest") != std::string::npos ||
                        loweredTask.find("price") != std::string::npos ||
                        loweredTask.find("news") != std::string::npos ||
                        loweredTask.find("stock") != std::string::npos);

    if (needsSearch) {
        std::string searchResult = performWebSearch(userTask);
        context += "\n\n[WEB SEARCH RESULTS]:\n" + searchResult;
    }
    // ---------------------------------------------

    std::string augmentedTask = userTask;
    if (!context.empty()) augmentedTask += "\n\n[CONTEXT]:\n" + context;

    // --- PHASE 1: PLANNER ---
    std::string plan = runAgent("planner", "Target: " + targetLang + "\nPlan for: " + augmentedTask);
    
    totalSwitchTime += engine.getLastStats().loadTimeMs;
    totalGenTime += engine.getLastStats().generationTimeMs;
    totalTokens += engine.getLastStats().tokensGenerated;

    std::cout << "\n[System] Plan generated. Coding...\n" << std::endl;
    
    // LIGHT 2: CODER (WRITING)
    currentStage = 2; 

    // --- PHASE 2: INITIAL CODE ---
    std::string currentCode = runAgent("coder", "Plan:\n" + plan + "\n\nWrite " + targetLang + " code.");
    
    totalSwitchTime += engine.getLastStats().loadTimeMs;
    totalGenTime += engine.getLastStats().generationTimeMs;
    totalTokens += engine.getLastStats().tokensGenerated;

    // --- PHASE 3: EXECUTION & REPAIR LOOP ---
    int max_retries = 2;
    std::string finalOutput = "";

    for (int i = 0; i < max_retries; i++) {
        // LIGHT 3: QA REVIEW
        currentStage = 3;
        
        std::cout << "\n[System] Cycle " << (i+1) << ": Saving & Executing..." << std::endl;
        
        std::string path = saveArtifact(currentCode, userTask, targetLang);
        
        if (targetLang == "Python") {
            std::string execOutput = executeScript(path, targetLang);
            std::cout << "[Sandbox Output]\n" << execOutput << std::endl;

            bool hasError = (execOutput.find("Traceback") != std::string::npos || 
                             execOutput.find("Error") != std::string::npos);

            if (!hasError) {
                std::cout << "\n[System] Execution SUCCESS! Verified working code." << std::endl;
                finalOutput = execOutput;
                break; 
            } else {
                std::cout << "\n[System] Execution FAILED. Requesting Fix..." << std::endl;
                
                currentStage = 2; 
                
                std::string fixPrompt = "The previous code failed with this error:\n" + execOutput + 
                                      "\n\nFix the code. Output ONLY the full corrected code.";
                
                currentCode = runAgent("coder", fixPrompt);
                
                totalSwitchTime += engine.getLastStats().loadTimeMs;
                totalGenTime += engine.getLastStats().generationTimeMs;
                totalTokens += engine.getLastStats().tokensGenerated;
            }
        } else {
            break;
        }
    }

    currentStage = 0;

    SwarmResult res;
    res.responseText = currentCode + "\n\n/* [SANDBOX OUTPUT]\n" + finalOutput + "\n*/";
    res.totalGenTimeSec = totalGenTime / 1000.0;
    res.totalSwitchTimeSec = totalSwitchTime / 1000.0;
    res.totalTokens = totalTokens;
    res.tokensPerSec = (totalGenTime > 0) ? (totalTokens / (totalGenTime / 1000.0)) : 0.0;

    return res;
}
