// Libtorch
#include "torch/torch.h"
#include "torch/script.h"
// Core
#include "include/wan.hpp"
using json = nlohmann::json;
using namespace nlohmann::literals;

int main(int argc, char *argv[])
{
    std::string HOST;
    int PORT;

    if (const auto env_host = std::getenv("WAN_SERVER_HOST"))
    {
        HOST = env_host;
    }
    else
    {
        HOST = "0.0.0.0";
        LOG(INFO) << std::endl
                  << "There is no environment variable called \"WAN_SERVER_HOST\"\n";
        LOG(INFO) << "Setting the HOST address to " << HOST << "\n";
    }

    if (const auto env_port = std::getenv("WAN_SERVER_PORT"))
    {
        std::stringstream value;
        value << env_port;
        value >> PORT;
    }
    else
    {
        PORT = 4004;
        LOG(INFO) << std::endl
                  << "There is no environment variable called \"WAN_SERVER_PORT\"\n";
        LOG(INFO) << "Setting the PORT to " << PORT << "\n";
    }
    wan::http::Server server(HOST, PORT);
    server.init();

    std::string model_path = "models/model.pt";
    wan::analytics::ClassificationService service(model_path);
    wan::analytics::ClassificationPipeline pipeline;

    // Classification Route
    server.request("/classify", "POST", [&pipeline, &service](auto req, auto args)
                   { pipeline.routeResponse(req, args, "POST", service); });

    // GET Invalid Route Handler
    server.request_regex(std::regex(R"(\D+)"), "GET", [&pipeline, &service](auto req, auto args)
                         { pipeline.invalidRoute(req, args, "GET"); });

    // POST Invalid Route Handler
    server.request_regex(std::regex(R"(\D+)"), "POST", [&pipeline, &service](auto req, auto args)
                         { pipeline.invalidRoute(req, args, "GET"); });

    server.run();
}
