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
        std::cout << " SENDING TO VALIDATE " << std::endl;

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
        std::cout << "Username: " << account->username() << " password: " << account->password() << std::endl;

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
        } else {
            reply->set_valid(false);
            reply->set_user_exists(true);
        }
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
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        PresentationLine narrator;
        PresentationLine line;
        std::string clear_screen = std::string(100, '\n');
        std::string screen_break = std::string(7, '\n');
        PresentationLine presentation_clear;
        PresentationLine presentation_break;
        presentation_clear.set_line(clear_screen);
        presentation_break.set_line(screen_break);


        writer->Write(presentation_clear);

        narrator.set_line("Welcome to the galaxy. Let's take a look at the stars!\n\n\nOne moment while we get your telescope set up...\n\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        writer->Write(narrator);
        writer->Write(presentation_break);
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        std::vector<std::string> telescope = client.GetArt("astronomer");
        for (auto s : telescope) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            line.set_line(s);
            writer->Write(line);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        narrator.set_line("Excellent. Now, let's try focusing that telescope on the north sky...");
        writer->Write(narrator);
        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
        writer->Write(presentation_break);
        std::vector<std::string> dipper = client.GetArt("dipper");
        for (auto s : dipper) {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            line.set_line(s);
            writer->Write(line);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        line.set_line("\n\nDo you see those? They're the Big and Little dippers.\n\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        line.set_line("In early myths, the seven stars of the Little Dipper represented the Hesperides, the nymphs tasked with guarding Hera's orchard.\n\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(7500));
        line.set_line("However, the nymphs would pluck immortality-granting apples from the orchard, and Hera placed the one-hundred-headed ever-awake dragon Ladon to watch them.\n\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(7500));
        line.set_line("Ladon can be found to this day as the constellation Draco, which neighbors the Little Dipper, ever watchful.\n\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(7500));
        line.set_line("That's enough about constellations.\n\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        writer->Write(presentation_break);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        std::vector<std::string> earth = client.GetArt("earth");
        for (auto s : earth) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            line.set_line(s);
            writer->Write(line);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        line.set_line("We'll make a quick scenic stop at Titan, Saturn's largest moon.\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        writer->Write(presentation_break);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        std::vector<std::string> titan = client.GetArt("titan");
        for (auto s : titan) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            line.set_line(s);
            writer->Write(line);
        }
        writer->Write(presentation_break);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        line.set_line("Titan is hugged by a thick, orange-colored smog-like atmosphere, and is the only moon in the solar system with a thick atmosphere.\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(7500));
        line.set_line("This atmosphere is so thick that at the surface of Titan, the atmospheric pressure is comparable to the pressure felt while swimming 15 meters under the ocean.\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(7500));
        line.set_line("This atmosphere is mostly composed of nitrogen and methane. The methane can condense into clouds that will drench the surface in methane storms.\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(7500));
        line.set_line("Vast, dark sand dunes composed of hydrocarbon grains stretch across Titan's landscape. These tall dunes are visually not unlike those in the deserts of our own Namibia.\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(7500));
        line.set_line("Despite this seeming inhospitality, Titan represents an incredibly unique opportunity in our solar system.\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(7500));
        line.set_line("The Cassini spacecraft has detected that Titan is likely hiding an underground ocean of liquid water.\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(7500));
        line.set_line("This global ocean may lie 35 to 50 miles underneath the icy ground. Naturally, this is intriguing to many.\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(7500));
        line.set_line("But that's enough about Titan. Onwards and upwards!.\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        writer->Write(presentation_clear);
        line.set_line("Exiting Titan's gravity.\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        writer->Write(presentation_break);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        line.set_line("Exiting our solar system. There's the sun way over there!\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::vector<std::string> sun = client.GetArt("sun");
        for (auto s : sun) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            line.set_line(s);
            writer->Write(line);
        }
        line.set_line("Engaging warp speed..........\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        line.set_line("\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(450));
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(330));
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(220));
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        writer->Write(presentation_clear);
        

        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        PresentationLine zoom_out;
        zoom_out.set_line("Now we can get one good look at the big picture.\n\n");
        writer->Write(zoom_out);
        writer->Write(presentation_break);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        std::vector<std::string> space = client.GetArt("space");

        for (auto s : space) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            line.set_line(s);
            writer->Write(line);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
        
        line.set_line("Thank yor for experiencing Cassiopeia.\n\n");
        writer->Write(line);
        line.set_line("https://github.com/Hablapatabla/little-dipper\n\n");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        line.set_line("All art is from: https://www.asciiart.eu/space.\n\nArt credits in particular go to:");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        line.set_line("Telescope - ejm97");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        line.set_line("The Two Dippers - Ojo");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        line.set_line("Saturn as seen from Titan - Robert Casey");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        line.set_line("Earth - JT");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        line.set_line("Sun - jgs");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        line.set_line("Universe - unknown");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        writer->Write(presentation_break);
        line.set_line("We are just an advanced breed of monkeys on a minor planet of a very average star. But we can understand the Universe. That makes us something very special.\n\t-Stephen Hawking");
        writer->Write(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
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
