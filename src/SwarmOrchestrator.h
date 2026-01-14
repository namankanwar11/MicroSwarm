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
    SwarmResult runIterativeTask(const std::string& rawInput);
    std::string runIoTDecision(const std::string& sensorData);
    int getCurrentStage() const { return currentStage; }

private:
    LLMEngine engine;
    std::map<std::string, Agent> agents;
    std::string currentLoadedModel = "";
    std::atomic<int> currentStage{0}; 

    std::map<std::string, double> modelWeights;   
    std::map<std::string, double> modelVelocity;  
    std::map<std::string, double> timingStats; 
    
    double learningRate = 0.1; 
    double momentum = 0.9;     

    std::string loadKnowledgeBaseRAG(const std::string& query);
    std::string saveArtifact(const std::string& content, const std::string& taskName, const std::string& lang);
    std::string executeScript(const std::string& filepath, const std::string& lang);
    std::string performWebSearch(const std::string& query);
    std::string runAgent(const std::string& agentId, const std::string& input);
    
    std::string selectBestModelForTask(const std::string& task);
    void updateModelMomentum(const std::string& modelName, double reward); 
    void updateModelNesterov(const std::string& modelName, double reward); 

    bool checkSkillLibrary(const std::string& task, std::string& outCode);
    void saveSkillToLibrary(const std::string& task, const std::string& code);
    std::string runMultiAgentDebate(const std::string& initialPlan, const std::string& task);
    std::string generateUnitTests(const std::string& plan, const std::string& lang);
    
    std::string runSecurityAudit(const std::string& code);
};