#include <grpcpp/grpcpp.h>
#include <string>
#include <iostream>
#include "spaceart.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReader;

using spaceart::Presenter;
using spaceart::Account;
using spaceart::CheckInResponse;
using spaceart::PresentationToken;
using spaceart::PresentationLine;

#include "tracer_common.hpp"

class Viewer {
public:
  Viewer(std::shared_ptr<Channel> channel) : stub_(Presenter::NewStub(channel)) {}
  std::string CheckIn(std::string username, std::string password) {
    std::string span_name = "Viewer Client - CheckIn RPC";
    auto span = get_tracer("Viewer")->StartSpan(span_name);
    auto scope = get_tracer("Viewer")->WithActiveSpan(span);
    Account account;
    account.set_username(username);
    account.set_password(password);
    CheckInResponse response;
    ClientContext context;

    Status status = stub_->CheckIn(&context, account, &response);
    if (status.ok()) {
        if (response.valid()) {
            //token = response.token();
            return "Good";
        } else {
            if (response.wrong_password() ) 
                return "Wrong";
            else 
                return "New";
        }
    } else {
        std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
        return "RPC Failed";
    }
    span->End();
  }

  void Present() {
    std::string span_name = "Viewer Client - Present RPC";
    auto span = get_tracer("Viewer")->StartSpan(span_name);
    auto scope = get_tracer("Viewer")->WithActiveSpan(span);
    PresentationLine line;
    ClientContext context;
    PresentationToken token;
    token.set_token("eggsbenedict");
    std::unique_ptr<ClientReader<PresentationLine> > reader(stub_->Present(&context, token));
    while (reader->Read(&line))
      std::cout << line.line() << std::endl;


    Status status = reader->Finish();
    span->End();
  }

  std::string SubmitAccount(std::string username, std::string password) {
    std::string span_name = "Viewer Client - SubmitAccount RPC";
    auto span = get_tracer("Viewer")->StartSpan(span_name);
    auto scope = get_tracer("Viewer")->WithActiveSpan(span);
    Account account;
    account.set_username(username);
    account.set_password(password);
    CheckInResponse response;
    ClientContext context;


    Status status = stub_->SubmitAccount(&context, account, &response);
    span->End();
    if (status.ok()) {
        if (response.valid()) {
            mytoken = response.token();
            return "Good";
        } else {
            if (response.user_exists()) {
                return "Exists";
            }
            else {
                return "Unknown";
            }
        }
    } else {
        std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
        return "RPC Failed";
    }
  }

private:
  std::unique_ptr<Presenter::Stub> stub_;
  std::string mytoken;
};


std::string GetUsersName() {
    std::string span_name = "Viewer Client - Get User Username";
    auto span = get_tracer("Viewer")->StartSpan(span_name);
    auto scope = get_tracer("Viewer")->WithActiveSpan(span);
    std::cout << "Let's start with a name. Enter a name that you'll remember now (please no whitespace characters!): " << std::endl;
    std::string name;
    std::cin >> name;
    span->End();
    return name;
}

std::string GetUsersPass() {
    std::string span_name = "Viewer Client - Get User Password";
    auto span = get_tracer("Viewer")->StartSpan(span_name);
    auto scope = get_tracer("Viewer")->WithActiveSpan(span);
    std::cout << "Next, set a password that you think you'll remember (please no whitespace characters!): " << std::endl;
    std::string pass;
    std::cin >> pass;
    span->End();
    return pass;
}

void RunViewer() {
    std::string span_name = "Viewer Client - RunViewer";
    auto span = get_tracer("Viewer")->StartSpan(span_name);
    auto scope = get_tracer("Viewer")->WithActiveSpan(span);

    std::string address("0.0.0.0:50052");
    Viewer viewer(grpc::CreateChannel(address, grpc::InsecureChannelCredentials()));

    std::cout << "Welcome to Cassiopeia. Is this your first time? [y, n]" << std::endl;
    
    std::string name;
    std::string pass;
    std::string input;
    std::cin >> input;

    if (input.compare("y") == 0 || input.compare("Y") == 0) {
        name = GetUsersName();
        pass = GetUsersPass();
        std::string stat = viewer.SubmitAccount(name, pass);
        if (stat.compare("Good") == 0) {
            std::cout << "You're all good to go! Handing things over to your presenter..." << std::endl;
            viewer.Present();
        }
        else if (stat.compare("Exists") == 0) {
            std::cout << "Unfortunately, we already have an account with that name. Let's start over, and please try registering with a different name." << std::endl;
            span->End();
            RunViewer();
        }
        else {
            std::cout << "STAT NOT GOOD" << std::endl;
            span->End();
        }
    }
    else if (input.compare("n") == 0 || input.compare("N") == 0) {
        std::cout << "Username:" << std::endl;
        std::cin >> name;
        std::cout << "Password: " << std::endl;
        std::cin >> pass;
        
        std::string stat = viewer.CheckIn(name, pass);
        if (stat.compare("New") == 0) {
            std::cout << "We have no account registered under that name, but we can enroll you now." << std::endl;
            name = GetUsersName();
            pass = GetUsersPass();
            std::string stat = viewer.SubmitAccount(name, pass);
            if (stat.compare("Good") == 0) {
                std::cout << "You're all good to go! Handing things over to your presenter..." << std::endl;
                viewer.Present();
            }
            else {
                std::cout << "STAT NOT GOOD 2" << std::endl;
            }
        }
        else if (stat.compare("Wrong") == 0) {
            std::cout << "Invalid password. Let's try starting over." << std::endl;
            span->End();
            RunViewer();
        }
        else if (stat.compare("Good") == 0) {
            std::cout << "You're all good to go! Handing things over to your presenter..." << std::endl << std::endl;
            viewer.Present();
        }
    }
    span->End();
}

int main(int argc, char** argv) {
  // TODO: TRACER REMOVED HEERE FOR EASE OF READING - ADD EXPORTER TO A FILE AT LEAST OR SOMETHING
  RunViewer();
  return 0;
}
