#include "APIServer.h"
#include "SwarmOrchestrator.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

// Helper to read file with multiple path checks
std::string readFile(const std::string& filename) {
    std::vector<std::string> paths = {
        "../" + filename,
        "../../" + filename,
        filename
    };

    for (const auto& path : paths) {
        std::ifstream f(path);
        if (f.is_open()) {
            std::stringstream buf;
            buf << f.rdbuf();
            std::cout << "[System] Loaded UI from: " << path << std::endl;
            return buf.str();
        }
    }

    std::cerr << "[Error] Could not find " << filename << std::endl;
    return "<h1>Error: dashboard.html not found. Check server logs.</h1>";
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: microswarm <model_planner> <model_coder>" << std::endl;
        return 1;
    }

    SwarmOrchestrator orchestrator;

    // ================= PLANNER =================
    orchestrator.registerAgent("planner", { 
        "Architect",
        argv[1],
        "You are a Senior System Architect. Analyze the user request and generate a numbered implementation plan. Be concise."
    });

    // ================= HACKER CODER =================
    orchestrator.registerAgent("coder", { 
        "Coder",
        argv[2],
        "You are an autonomous AI programming engine. "
        "Your goal is to write EXECUTABLE, COMPLETE Python scripts. "
        "DO NOT write comments. DO NOT write docstrings. DO NOT explain the code. "
        "Start directly with imports or the main logic. "
        "Ensure the code prints the final result to stdout so it can be verified."
    });

    APIServer server(8080);

    server.post("/api/swarm", [&](const std::string& body) {
        std::cout << "\n[API] Received Task: " << body << std::endl;
        SwarmResult res = orchestrator.runIterativeTask(body);
        return res.responseText;
    });

    server.get("/status", [&](const std::string&) {
        return "{\"status\":\"online\"}";
    });

    server.get("/", [&](const std::string&) {
        return readFile("dashboard.html");
    });

    std::cout << "[Main] Initializing Heterogeneous Swarm..." << std::endl;
    server.start();

    return 0;
}
