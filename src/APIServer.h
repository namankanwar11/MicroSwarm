#pragma once
#include "httplib.h"
#include "SwarmOrchestrator.h"
#include <string>
#include <iostream>

class APIServer {
public:
    APIServer(SwarmOrchestrator* swarm) : swarm(swarm) {
        setupRoutes();
    }

    void start(int port) {
        std::cout << "[Server] Listening on http://localhost:" << port << "..." << std::endl;
        server.listen("0.0.0.0", port);
    }

private:
    httplib::Server server;
    SwarmOrchestrator* swarm;

    void setupRoutes() {
        // Enable CORS (Cross-Origin Resource Sharing) so web browsers can talk to us
        server.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type");
            if (req.method == "OPTIONS") {
                res.status = 200;
                return httplib::Server::HandlerResponse::Handled;
            }
            return httplib::Server::HandlerResponse::Unhandled;
        });

        // Endpoint: /status
        server.Get("/status", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("{\"status\": \"MicroSwarm Online\", \"memory\": \"OK\"}", "application/json");
        });

        // Endpoint: /agent_status  (Who is working right now?)
        server.Get("/agent_status", [this](const httplib::Request&, httplib::Response& res) {
            int stage = this->swarm->getCurrentStage();
            res.set_content(std::to_string(stage), "text/plain");
        });

        // Endpoint: /run_task
        server.Post("/run_task", [this](const httplib::Request& req, httplib::Response& res) {
            std::string task = req.body;
            std::cout << "\n[API] Received Task: " << task << std::endl;

            // Run the Swarm and get the metrics struct
            SwarmResult result = this->swarm->runIterativeTask(task);

            // Send performance telemetry in HTTP headers
            res.set_header("X-Gen-Time", std::to_string(result.totalGenTimeSec));
            res.set_header("X-Switch-Time", std::to_string(result.totalSwitchTimeSec));
            res.set_header("X-Token-Count", std::to_string(result.totalTokens));
            res.set_header("X-Token-Speed", std::to_string(result.tokensPerSec));

            // Send the generated code as the response body
            res.set_content(result.responseText, "text/plain");
        });
    }
};
