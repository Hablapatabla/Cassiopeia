#include <grpcpp/grpcpp.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <string>
#include <fstream>
#include <chrono>
#include <thread>
#include <filesystem> //DELETE THIS
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
        opentelemetry::trace::StartSpanOptions options;
        options.kind = opentelemetry::trace::SpanKind::kServer;
        std::string span_name = "Registrar/Validate";
        auto span = get_tracer("registrar")
                    ->StartSpan(span_name,
                                {{"rpc.system", "grpc"},
                                 {"rpc.service", "cassiopeia.Registrar"},
                                 {"rpc.method", "Validate"},
                                 {"rpc.net.peer.ip", "localhost"},
                                 {"rpc.net.peer.port", 50053},
                                 {"rpc.net.peer.name", "localhost"},
                                 {"rpc.net.peer.transport", "ip_tcp"},
                                 {"rpc.grpc.status_code", 0},
                                 {"account.username", account->username()},
                                 {"account.password", account->password()}},
                                options);
        auto scope = get_tracer("registrar")->WithActiveSpan(span);

        RegistryStatus rs = check_account(account->username(), account->password());
        switch(rs) {
          case Valid : 
            {
              response->set_valid(true);
              break;
            }
          case Invalid :
            {
              response->set_valid(false);
              break;
            }
          case Password :
            {
              response->set_valid(false);
              response->set_wrong_password(true);
              break;
            }
          default :
            {
              std::cout << "Invalid enumeration type returned" << std::endl;
              break;
            }
        }
        
        span->End();
        return Status::OK;
    }

  Status Register(ServerContext* context,
                      const Account* account,
                      AuthResponse* response) override
    {
        opentelemetry::trace::StartSpanOptions options;
        options.kind = opentelemetry::trace::SpanKind::kServer;
        std::string span_name = "Registrar/Register";
        auto span = get_tracer("registrar")
                    ->StartSpan(span_name,
                                {{"rpc.system", "grpc"},
                                 {"rpc.service", "cassiopeia.Registrar"},
                                 {"rpc.method", "Register"},
                                 {"rpc.net.peer.ip", "localhost"},
                                 {"rpc.net.peer.port", 50053},
                                 {"rpc.net.peer.name", "localhost"},
                                 {"rpc.net.peer.transport", "ip_tcp"},
                                 {"rpc.grpc.status_code", 0}},
                                options);
        auto scope = get_tracer("registrar")->WithActiveSpan(span);

        std::this_thread::sleep_for(std::chrono::milliseconds(1800));

        RegistryStatus rs = check_if_user_already_exists(account->username());
        switch (rs) {
          case Valid :
            {
              std::stringstream ss;
              ss << account->username() << ' ' << account->password() << std::endl;
              std::ofstream registrar_file;
              registrar_file.open(registrar_path, std::ios::app);
              if (registrar_file.is_open()) {
                  registrar_file << ss.rdbuf();
                  registrar_file.close();
              }
              else {
                std::cout << "Couldn't open registry" << std::endl;
              }
              response->set_valid(true);
              break;
            }
          case Exists :
            {
              response->set_valid(false);
              response->set_user_exists(true);
              break;
            }
          default :
            {
              response->set_valid(false);
              break;
            }
        }

        span->End();
        return Status::OK;
    }
  private:
    enum RegistryStatus { Valid, Invalid, Password, Exists };
    const std::string registrar_path = "../../registry/registry.txt";

    RegistryStatus check_if_user_already_exists(std::string account_user) {
      std::string line;
      std::string user;
      std::string pass;
      std::ifstream file;
      file.open(registrar_path);

      if (file.is_open()) {
          while (getline(file, line)) {
              std::stringstream ss(line);
              getline(ss, user, ' ');
              getline(ss, pass, '\n');
              
              if (user.compare(account_user) == 0) {
                  file.close();
                  return Exists;
              }
          }
      }
      file.close();
      return Valid;
    }

    RegistryStatus check_account(std::string account_user, std::string account_pass) {
        std::string user;
        std::string pass;
        std::string line;

        std::ifstream registrar_file;
        registrar_file.open(registrar_path);

        if (registrar_file.is_open()) {
            while (getline(registrar_file, line)) {
                std::stringstream ss(line);
                getline(ss, user, ' ');
                getline(ss, pass, '\n');

                if (user.compare(account_user) == 0) {
                    if (pass.compare(account_pass) == 0) {
                      registrar_file.close();
                      return Valid;
                    }
                    else {
                      registrar_file.close();
                      return Password;
                    }
                }
            }
        }
        registrar_file.close();
        return Invalid;
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