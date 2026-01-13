#pragma once

#include "LLMEngine.h"
#include "Agent.h"
#include <map>
#include <vector>
#include <string>
#include <atomic> 

struct SwarmResult {
    std::string responseText;
    double totalGenTimeSec;
    double totalSwitchTimeSec;
    int totalTokens;
    double tokensPerSec;
};

class SwarmOrchestrator {
public:
    SwarmOrchestrator();
    
    void registerAgent(const std::string& id, const Agent& agent);
    std::string runAgent(const std::string& agentId, const std::string& input);
    SwarmResult runIterativeTask(const std::string& rawInput);
    int getCurrentStage() const { return currentStage; }

private:
    LLMEngine engine;
    std::map<std::string, Agent> agents;
    std::string currentLoadedModel = "";
    std::atomic<int> currentStage{0}; 

    std::string loadKnowledgeBase();
    std::string saveArtifact(const std::string& content, const std::string& taskName, const std::string& lang);
    
    // Function to execute the saved script and get output
    std::string executeScript(const std::string& filepath, const std::string& lang);
    
    // NEW: Function to run the Python search tool
    std::string performWebSearch(const std::string& query);
};
