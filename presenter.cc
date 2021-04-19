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

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReader;

using spaceart::Presenter;
using spaceart::Account;
using spaceart::Gallery;
using spaceart::Registrar;
using spaceart::CheckInResponse;
using spaceart::PresentationToken;
using spaceart::PresentationLine;
using spaceart::AuthResponse;
using spaceart::ArtLine;
using spaceart::ArtRequest;

#include "tracer_common.hpp"

class RegistrarClient {
public:
  RegistrarClient(std::shared_ptr<Channel> channel) : stub_(Registrar::NewStub(channel)) {}

    AuthResponse Validate(Account account) {
        std::string span_name = "Registrar Client - Validate RPC";
        auto span = get_tracer("Presenter")->StartSpan(span_name);
        auto scope = get_tracer("Presenter")->WithActiveSpan(span);
        ClientContext context;
        AuthResponse response;

        Status status = stub_->Validate(&context, account, &response);
        span->End();
        return response;
    }

    AuthResponse Register(Account account) {
        std::string span_name = "Registrar Client - Register RPC";
        auto span = get_tracer("Presenter")->StartSpan(span_name);
        auto scope = get_tracer("Presenter")->WithActiveSpan(span);
        ClientContext context;
        AuthResponse response;

        Status status = stub_->Register(&context, account, &response);
        span->End();
        return response;
    }


private:
  std::unique_ptr<Registrar::Stub> stub_;
};

class GalleryClient {
public:
  GalleryClient(std::shared_ptr<Channel> channel) : stub_(Gallery::NewStub(channel)) {}

    std::vector<std::string> GetArt(std::string name) {
        std::string span_name = "Gallery Client - GetArt RPC";
        auto span = get_tracer("Presenter")->StartSpan(span_name);
        auto scope = get_tracer("Presenter")->WithActiveSpan(span);
        std::vector<std::string> v;
        ClientContext context;
        ArtRequest request;
        request.set_name(name);
        std::unique_ptr<ClientReader<ArtLine> > reader(stub_->GetArt(&context, request));

        ArtLine line;
        while (reader->Read(&line))
            v.push_back(line.line());

        Status status = reader->Finish();
        span->End();
        return v;
    }

private:
  std::unique_ptr<Gallery::Stub> stub_;
};

class PresenterServer final : public Presenter::Service {
public :
    Status CheckIn(ServerContext* context,
                        const Account* account,
                        CheckInResponse* reply) override
    {
        std::string span_name = "Presenter Service - CheckIn RPC";
        auto span = get_tracer("Presenter")->StartSpan(span_name);
        auto scope = get_tracer("Presenter")->WithActiveSpan(span);
        std::string username = account->username();
        std::string password = account->password();
        
        // 52 for registrar
        RegistrarClient client(grpc::CreateChannel(registrar_port, grpc::InsecureChannelCredentials()));

        if (password.compare("") == 0) {
            reply->set_valid(false);
            reply->set_new_password_desired(true);
            span->End();
            return Status::OK;
        }
        else {
            AuthResponse authr = client.Validate(*account);
            if (authr.valid()) {
                reply->set_valid(true);
                reply->set_token(pass);
                span->End();
                return Status::OK;
            }
            else {
                if (authr.wrong_password()) {
                    reply->set_valid(false);
                    reply->set_new_password_desired(true);
                    span->End();
                    return Status::OK;
                } else {
                    reply->set_valid(false);
                    reply->set_wrong_password(true);
                    span->End();
                    return Status::OK;
                }
            }
        }
    }

    Status SubmitAccount(ServerContext* context,
                        const Account* account,
                        CheckInResponse* reply) override
    {
        std::string span_name = "Presenter Service - SubmitAccount RPC";
        auto span = get_tracer("Presenter")->StartSpan(span_name);
        auto scope = get_tracer("Presenter")->WithActiveSpan(span);
        RegistrarClient client(grpc::CreateChannel(registrar_port, grpc::InsecureChannelCredentials()));
        AuthResponse ar = client.Register(*account);
        if (ar.valid()) {
            reply->set_valid(true);
            reply->set_token(pass);
            return Status::OK;
        }
        // Not ok!
        span->End();
        return Status::OK;
    }

    Status Present(ServerContext* context,
                        const PresentationToken* pt,
                        ServerWriter<PresentationLine>* writer) override
    {   
        std::string span_name = "Presenter Service - Present RPC";
        auto span = get_tracer("Presenter")->StartSpan(span_name);
        auto scope = get_tracer("Presenter")->WithActiveSpan(span);
        GalleryClient client(grpc::CreateChannel(gallery_port, grpc::InsecureChannelCredentials()));
        
        PresentationLine intro;
        intro.set_line("Welcome to the galaxy. Let's take a look at the stars!\n\n\nOne moment while we get your telescope set up...\n\n");
        writer->Write(intro);
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        std::vector<std::string> telescope = client.GetArt("astronomer");

        PresentationLine line;
        for (auto s : telescope) {
            std::this_thread::sleep_for(std::chrono::milliseconds(450));
            line.set_line(s);
            writer->Write(line);
        }

        PresentationLine zoom_out;
        zoom_out.set_line("Now we can finally start to see the big picture.\n\n\nZooming out...\n\n");
        writer->Write(zoom_out);
        std::this_thread::sleep_for(std::chrono::milliseconds(800));

        std::vector<std::string> space = client.GetArt("space");

        for (auto s : space) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            line.set_line(s);
            writer->Write(line);
        }
        span->End();
        return Status::OK;
    }
    private:
        std::string pass =                 "eggsbenedict";
        const std::string registrar_port = "0.0.0.0:50053";
        const std::string gallery_port =   "0.0.0.0:50054";
};


void RunServer() {
    std::string address("0.0.0.0:50052");
    PresenterServer service;
    ServerBuilder builder;

    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on port: " << address << std::endl;
    server->Wait();
}

int main(int argc, char** argv) {
    initTracer();
    auto root_span = get_tracer("Presenter")->StartSpan(__func__);
    opentelemetry::trace::Scope scope(root_span);

    RunServer();
    root_span->End();
    return 0;
}
