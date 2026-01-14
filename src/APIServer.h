#pragma once

#include <string>
#include <functional>
#include <iostream>
#include "httplib.h"

class APIServer {
public:
    APIServer(int port) : port_(port) {}

    // =========================
    // Register GET route (CORS enabled)
    // =========================
    void get(const std::string& path, std::function<std::string(const std::string&)> handler) {
        svr_.Get(path, [handler, path](const httplib::Request& req, httplib::Response& res) {
            // ENABLE CORS
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Headers", "*");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");

            std::string response = handler(req.body);

            // Content-Type routing
            if (path == "/") {
                res.set_content(response, "text/html");
            } 
            else if (path == "/status") {
                res.set_content(response, "application/json");
            } 
            else {
                res.set_content(response, "text/plain");
            }
        });
    }

    // =========================
    // Register POST route (CORS enabled)
    // =========================
    void post(const std::string& path, std::function<std::string(const std::string&)> handler) {
        svr_.Post(path, [handler](const httplib::Request& req, httplib::Response& res) {
            // ENABLE CORS
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Headers", "*");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");

            std::string response = handler(req.body);
            res.set_content(response, "text/plain");
        });
    }

    // =========================
    // Start Server
    // =========================
    void start() {
        std::cout << "[Server] Listening on http://localhost:" << port_ << "..." << std::endl;
        svr_.listen("0.0.0.0", port_);
    }

private:
    int port_;
    httplib::Server svr_;
};
