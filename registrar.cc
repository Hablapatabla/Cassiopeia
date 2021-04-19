#include <grpcpp/grpcpp.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <string>
#include <fstream>
#include <chrono>
#include <thread>
#include "spaceart.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;


using spaceart::Presenter;
using spaceart::Account;
using spaceart::Registrar;
using spaceart::AuthResponse;

#include "tracer_common.hpp"

class RegistrarServer final : public Registrar::Service {
  public:
  Status Validate(ServerContext* context,
                      const Account* account,
                      AuthResponse* response) override
    {
        std::string span_name = "Registrar Service - Validate RPC";
        auto span = get_tracer("Registrar")->StartSpan(span_name);
        auto scope = get_tracer("Registrar")->WithActiveSpan(span);
        std::cout << "Stubbed" << std::endl;
        response->set_valid(true);
        span->End();
        return Status::OK;
    }

  Status Register(ServerContext* context,
                      const Account* account,
                      AuthResponse* response) override
    {
        std::string span_name = "Registrar Service - Register RPC";
        auto span = get_tracer("Gallery")->StartSpan(span_name);
        auto scope = get_tracer("Gallery")->WithActiveSpan(span);
        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
        std::cout << "Stubbed" << std::endl;
        response->set_valid(true);
        span->End();
        return Status::OK;
    }
};


void RunServer() {
  std::string address("0.0.0.0:50053");
  RegistrarServer service;
  ServerBuilder builder;

  builder.AddListeningPort(address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on port: " << address << std::endl;
  server->Wait();
}

int main(int argc, char** argv) {
  initTracer();
  auto root_span = get_tracer("Registrar")->StartSpan(__func__);
  opentelemetry::trace::Scope scope(root_span);
  RunServer();
  root_span->End();
  return 0;
}