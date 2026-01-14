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
#include <thread> 

namespace fs = std::filesystem;
using namespace std::chrono;

SwarmOrchestrator::SwarmOrchestrator() {
    std::cout << "[Orchestrator] System initialized." << std::endl;
    modelWeights["phi-3"] = 0.5;
    modelWeights["qwen"] = 0.5;
    modelVelocity["phi-3"] = 0.0;
    modelVelocity["qwen"] = 0.0;
    
    if (!fs::exists("../skills")) fs::create_directory("../skills");
}

void SwarmOrchestrator::registerAgent(const std::string& id, const Agent& agent) {
    agents.insert({id, agent});
}

std::string SwarmOrchestrator::loadKnowledgeBaseRAG(const std::string& query) {
    std::string toolScript = "../tools/rag_engine.py";
    if (!fs::exists(toolScript)) return "";

    std::string cmd = "python \"" + toolScript + "\" \"" + query + "\"";
    std::array<char, 128> buffer;
    std::string result;
    
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
    if (!pipe) return "";

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string cleanMarkdown(const std::string& content) {
    std::string cleanCode = content;
    size_t start = cleanCode.find("```");
    if (start != std::string::npos) {
        size_t end = cleanCode.find("\n", start);
        cleanCode = cleanCode.substr(end + 1); 
        size_t last = cleanCode.rfind("```");
        if (last != std::string::npos) cleanCode = cleanCode.substr(0, last); 
    }
    return cleanCode;
}

std::string SwarmOrchestrator::saveArtifact(const std::string& content, const std::string& taskName, const std::string& lang) {
    std::string outDir = "../output";
    if (!fs::exists(outDir)) fs::create_directory(outDir);

    std::string ext = (lang == "Python") ? ".py" : ".txt";
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << outDir << "/Gen_" << std::put_time(std::localtime(&now_c), "%H%M%S") << ext;
    
    std::ofstream outFile(ss.str());
    if (outFile.is_open()) {
        outFile << cleanMarkdown(content);
        return fs::absolute(ss.str()).string(); 
    }
    return "";
}

std::string SwarmOrchestrator::executeScript(const std::string& filepath, const std::string& lang) {
    if (lang != "Python") return "";
    std::string cmd = "python \"" + filepath + "\" 2>&1";
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
    if (!pipe) return "Error";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) result += buffer.data();
    return result;
}

std::string SwarmOrchestrator::performWebSearch(const std::string& query) {
    std::string toolScript = "../tools/search.py";
    if (!fs::exists(toolScript)) return "";
    std::string cmd = "python \"" + toolScript + "\" \"" + query + "\"";
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) result += buffer.data();
    return result;
}

std::string SwarmOrchestrator::selectBestModelForTask(const std::string& task) {
    if (modelWeights["phi-3"] > modelWeights["qwen"]) return "phi-3";
    return "qwen";
}

void SwarmOrchestrator::updateModelMomentum(const std::string& modelName, double reward) {
    std::string key = (modelName.find("phi") != std::string::npos) ? "phi-3" : "qwen";
    double gradient = (reward > 0) ? 1.0 : -1.0;
    modelVelocity[key] = (momentum * modelVelocity[key]) + (learningRate * gradient);
    modelWeights[key] += modelVelocity[key];
}

void SwarmOrchestrator::updateModelNesterov(const std::string& modelName, double reward) {
    std::string key = (modelName.find("phi") != std::string::npos) ? "phi-3" : "qwen";
    double gradient = (reward > 0) ? 1.0 : -1.0;
    double prev_v = modelVelocity[key];
    modelVelocity[key] = (momentum * prev_v) + (learningRate * gradient);
    modelWeights[key] += (-momentum * prev_v) + ((1 + momentum) * modelVelocity[key]);
}

std::string SwarmOrchestrator::runAgent(const std::string& agentId, const std::string& input) {
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

std::string SwarmOrchestrator::runIoTDecision(const std::string& sensorData) {
    std::string systemPrompt = 
        "You are an IoT Control Unit. Output ONLY a JSON command. "
        "Format: {\"device\": \"<name>\", \"action\": \"<ON/OFF>\", \"reason\": \"<brief>\"}";

    if (currentLoadedModel.find("phi-3") == std::string::npos) {
        Agent& a = agents.at("planner");
        engine.loadModel(a.modelPath);
        currentLoadedModel = a.modelPath;
    }

    std::stringstream prompt;
    prompt << "<|system|>\n" << systemPrompt << "<|end|>\n";
    prompt << "<|user|>\nData: " << sensorData << "<|end|>\n";
    prompt << "<|assistant|>\n";
    
    std::string response = engine.query(prompt.str(), 128);
    return cleanMarkdown(response);
}

bool SwarmOrchestrator::checkSkillLibrary(const std::string& task, std::string& outCode) {
    size_t hash = std::hash<std::string>{}(task);
    std::string filename = "../skills/skill_" + std::to_string(hash) + ".py";
    if (fs::exists(filename)) {
        std::ifstream f(filename);
        std::stringstream buffer;
        buffer << f.rdbuf();
        outCode = buffer.str();
        return true;
    }
    return false;
}

void SwarmOrchestrator::saveSkillToLibrary(const std::string& task, const std::string& code) {
    size_t hash = std::hash<std::string>{}(task);
    std::string filename = "../skills/skill_" + std::to_string(hash) + ".py";
    std::ofstream outFile(filename);
    if (outFile.is_open()) {
        outFile << cleanMarkdown(code);
    }
}

std::string SwarmOrchestrator::runMultiAgentDebate(const std::string& initialPlan, const std::string& task) {
    std::string currentPlan = initialPlan;
    std::cout << ">>> [War Room] Initiating Debate..." << std::endl;
    
    std::string critique = runAgent("coder", 
        "Plan:\n" + currentPlan + "\nTask: " + task + 
        "\n\nCRITIQUE this plan. Identify 1 logical flaw. Be brief."
    );
    std::cout << "    [Engineer] Critique: " << critique.substr(0, 100) << "..." << std::endl;

    std::string refinedPlan = runAgent("planner", 
        "Original Plan:\n" + currentPlan + 
        "\nEngineer Critique:\n" + critique + 
        "\n\nUpdate the plan to address the critique."
    );
    
    return refinedPlan;
}

std::string SwarmOrchestrator::generateUnitTests(const std::string& plan, const std::string& lang) {
    if (lang != "Python") return "";
    return runAgent("planner", 
        "Plan:\n" + plan + 
        "\n\nWrite 2 simple Python `assert` statements to verify the logic."
        "RULES:\n"
        "1. DO NOT check file existence or file names.\n"
        "2. DO NOT use open(), read(), or write().\n"
        "3. Only check function outputs or logic."
    );
}

std::string SwarmOrchestrator::runSecurityAudit(const std::string& code) {
    std::string prompt = 
        "You are a Red Team Security Expert. Analyze this code for vulnerabilities.\n"
        "Code:\n" + code + "\n\n"
        "If safe, output exactly: SAFE\n"
        "If unsafe, list the vulnerabilities (e.g., 'SQL Injection', 'RCE').";
        
    return runAgent("planner", prompt);
}

std::string generateTimeGraph(const std::map<std::string, double>& stats) {
    std::stringstream ss;
    ss << "\n[SYSTEM METRICS - EXECUTION TIME]\n";
    double maxTime = 0.1; 
    for(const auto& pair : stats) if(pair.second > maxTime) maxTime = pair.second;

    for (const auto& pair : stats) {
        ss << std::left << std::setw(12) << pair.first << " | ";
        int bars = (int)((pair.second / maxTime) * 30.0); 
        for(int i=0; i<bars; ++i) ss << "#";
        ss << " " << std::fixed << std::setprecision(2) << pair.second << "s\n";
    }
    return ss.str();
}

SwarmResult SwarmOrchestrator::runIterativeTask(const std::string& rawInput) {
    std::string targetLang = "C++"; 
    std::string userTask = rawInput;
    size_t delimiterPos = rawInput.find("|");
    if (delimiterPos != std::string::npos) {
        targetLang = rawInput.substr(0, delimiterPos);
        userTask = rawInput.substr(delimiterPos + 1);
    }

    std::cout << "\n=== STARTING EVOLUTIONARY SWARM [" << targetLang << "] ===\n" << std::endl;
    
    timingStats.clear();
    auto globalStart = high_resolution_clock::now();

    std::string cachedCode;
    if (checkSkillLibrary(userTask, cachedCode)) {
        std::cout << "[Skill Library] Task recognized. Retrieving solution." << std::endl;
        timingStats["Memory Fetch"] = 0.01;
        SwarmResult res;
        res.responseText = cachedCode + "\n\n/* [RETRIEVED FROM SKILL LIBRARY] */\n" + generateTimeGraph(timingStats);
        return res;
    }

    bool isComplexTask = userTask.length() > 50; 

    auto t1 = high_resolution_clock::now();
    currentStage = 1; 
    
    std::string context = loadKnowledgeBaseRAG(userTask);
    std::string plan = runAgent("planner", "Context:\n" + context + "\nTask: " + userTask);
    
    if (isComplexTask) {
        plan = runMultiAgentDebate(plan, userTask);
    } else {
        std::cout << "[System] Simple Task Detected -> Skipping Debate (Turbo Mode)" << std::endl;
    }
    
    std::string unitTests = "";
    if (isComplexTask) {
        unitTests = generateUnitTests(plan, targetLang);
        std::cout << ">>> [CodeT] Tests Generated." << std::endl;
    }

    auto t2 = high_resolution_clock::now();
    timingStats["Architect"] = duration<double>(t2 - t1).count();

    std::cout << "\n[System] Plan Ready. Evolving Code Population...\n" << std::endl;

    currentStage = 2; 
    int populationSize = isComplexTask ? 3 : 1; 
    std::string bestCode = "";
    std::string bestOutput = "";
    bool success = false;
    std::string auditReport = "";

    double totalCodingTime = 0;
    double totalQATime = 0;

    for(int i=0; i < populationSize; ++i) {
        auto t_code_start = high_resolution_clock::now();
        std::cout << ">>> [Genetic Algo] Spawning Variant " << (i+1) << "..." << std::endl;
        
        std::string variantPrompt = "Plan:\n" + plan + "\nWrite " + targetLang + " code. Variation: " + std::to_string(i);
        std::string code = runAgent("coder", variantPrompt);
        totalCodingTime += duration<double>(high_resolution_clock::now() - t_code_start).count();
        
        bestCode = code;

        currentStage = 3; 
        auto t_qa_start = high_resolution_clock::now();
        std::cout << ">>> [Genetic Algo] Testing Variant " << (i+1) << "..." << std::endl;
        
        std::string testableCode = code;
        if (!unitTests.empty()) {
            testableCode += "\n\n# --- UNIT TESTS ---\n" + cleanMarkdown(unitTests);
        }

        std::string path = saveArtifact(testableCode, userTask, targetLang);
        
        if(targetLang == "Python") {
            std::string out = executeScript(path, targetLang);
            totalQATime += duration<double>(high_resolution_clock::now() - t_qa_start).count();

            if(out.find("Error") == std::string::npos && out.find("Traceback") == std::string::npos) {
                std::cout << ">>> [Genetic Algo] Variant " << (i+1) << " Passed Functional Tests." << std::endl;
                
                std::cout << ">>> [Red Team] Initiating Security Scan..." << std::endl;
                std::string audit = runSecurityAudit(code);
                
                if (audit.find("SAFE") != std::string::npos) {
                    std::cout << "    [Red Team] Code is CLEAN." << std::endl;
                    bestOutput = out;
                    success = true;
                    updateModelMomentum("qwen", 1.0); 
                    saveSkillToLibrary(userTask, code);
                    auditReport = "SECURITY AUDIT: PASSED (No Vulnerabilities Found)";
                    break;
                } else {
                    std::cout << "    [Red Team] VULNERABILITY DETECTED!" << std::endl;
                    std::cout << "    Report: " << audit << std::endl;
                    updateModelMomentum("qwen", -0.5); 
                }
            } else {
                std::cout << ">>> [Genetic Algo] Variant " << (i+1) << " DIED (Logic Error)." << std::endl;
                updateModelMomentum("qwen", -0.5);
            }
        } else {
            success = true; 
            break;
        }
        currentStage = 2; 
    }

    timingStats["Evolution"] = totalCodingTime;
    timingStats["Sandbox QA"] = totalQATime;

    if (!success && targetLang == "Python") {
        std::cout << "\n[System] All variants failed. Attempting Self-Repair..." << std::endl;
        auto t_fix = high_resolution_clock::now();
        std::string repairPrompt = "The code failed:\n" + bestCode + "\n\nFix the errors and return ONLY the corrected code.";
        bestCode = runAgent("coder", repairPrompt);
        timingStats["Self-Repair"] = duration<double>(high_resolution_clock::now() - t_fix).count();
    }

    currentStage = 0;
    std::string timeGraph = generateTimeGraph(timingStats);
    SwarmResult res;
    res.responseText = bestCode + "\n\n/* [OUTPUT]\n" + bestOutput + "\n\n" + auditReport + "\n*/\n" + timeGraph;
    res.totalGenTimeSec = engine.getLastStats().generationTimeMs / 1000.0;
    res.totalTokens = engine.getLastStats().tokensGenerated;
    return res;
}