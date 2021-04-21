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
using spaceart::Gallery;
using spaceart::CheckInResponse;
using spaceart::PresentationToken;
using spaceart::PresentationLine;
using spaceart::ArtLine;
using spaceart::ArtRequest;

#include "tracer_common.hpp"

class GalleryServer final : public Gallery::Service {
public:
  Status GetArt(ServerContext* context,
                      const spaceart::ArtRequest* request,
                      ServerWriter<ArtLine>* writer) override
  {
    std::string art_name = request->name();
    std::string art_file_name = "../../art/" + art_name + ".txt";
    std::ifstream art_file(art_file_name);
    std::string span_name = "Gallery Service - GetArt RPC - " + art_name;
    auto span = get_tracer("Gallery")->StartSpan(span_name);
    auto scope = get_tracer("Gallery")->WithActiveSpan(span);
   
    if (!art_file.is_open()) {
      std::cout << "Couldn't open file" << std::endl;
      return grpc::Status(grpc::StatusCode::NOT_FOUND, "Could not find art with that name");
    }

    std::string text_line;
    while (getline(art_file, text_line)) {
      ArtLine artline;
      artline.set_line(text_line);
      writer->Write(artline);
    }
    art_file.close();
    span->End();
    return Status::OK;
  }
};


void RunServer() {
  std::string address("0.0.0.0:50054");
  GalleryServer service;
  ServerBuilder builder;

  builder.AddListeningPort(address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on port: " << address << std::endl;
  server->Wait();
}

int main(int argc, char** argv) {
  initTracer();
  auto root_span = get_tracer("Gallery")->StartSpan(__func__);
  opentelemetry::trace::Scope scope(root_span);
  RunServer();
  root_span->End();
  return 0;
}