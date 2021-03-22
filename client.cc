#include <grpcpp/grpcpp.h>
#include <string>
#include <iostream>
#include "spaceart.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReader;

using spaceart::SpaceArt;
using spaceart::ArtRequest;
using spaceart::ArtLine;

class SpaceArtClient {
public:
  SpaceArtClient(std::shared_ptr<Channel> channel) : stub_(SpaceArt::NewStub(channel)) {}

  void DisplayArt(std::string art_name) {
    ArtLine line;
    ClientContext context;
    ArtRequest request;
    request.set_name(art_name);

    std::unique_ptr<ClientReader<ArtLine> > reader(stub_->DisplayArt(&context, request));
    while (reader->Read(&line))
      std::cout << line.line() << std::endl;


    Status status = reader->Finish();
    if (status.ok()) {
      std::cout << "DisplayArt rpc succeeded." << std::endl;
    } else {
      std::cout << "DisplayArt rpc failed." << std::endl;
    }
  }
private:
  std::unique_ptr<SpaceArt::Stub> stub_;
};


void RunClient() {
  std::string address("0.0.0.0:50051");
  SpaceArtClient client(grpc::CreateChannel(address, grpc::InsecureChannelCredentials()));

  std::string name;
  while (1 == 1) {
    std::cout << "Which piece would you like to request? Enter astronomer, space, wormhole, or quit to end" << std::endl;
    std::cin >> name;
    if (name.compare("quit") == 0)
      break;
    client.DisplayArt(name);
  }
}

int main(int argc, char** argv) {
  RunClient();

  return 0;
}
