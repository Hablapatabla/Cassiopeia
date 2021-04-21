# Cassiopeia - An OpenTelemetry-instrumented in-command-line Experience

## Getting Started
Cassiopeia requires OpenTelemetry to be installed on your machine in order to work. You will need to install OpenTelemetry-cpp as well as its dependencies.

### Installing Dependencies
On a Mac, you can use Homebrew to install all required dependencies.
'brew install grpc'
'brew install protobuf'
'brew install cmake'
'brew install google-benchmark'
'brew tap nlohmann/json'
'brew install nlohmann-json'

Follow the instructions [here](https://github.com/open-telemetry/opentelemetry-cpp/blob/main/INSTALL.md) to install OpenTelemetry.

### Installing
Start off by cloning this repository wherever you may like. If you are unsure where to place it you can try:
'cd ~'
'mkdir source && cd source'
'gh repo clone Hablapatabla/little-dipper'

### Building
Navigate to the /cmake/build directory from the root directory.
'cd little-dipper'
'cd cmake/build'

If the directory has anything in it other than clean-cmake.sh, run:
'bash clean-cmake.sh'.

Now, you can build the project using CMake. You should be able to simply run:
'cmake ../..'
'make'

If the project built successfully, you should have four executables:
./viewer    ./presenter     ./registrar     ./gallery

These executables are the four services in Cassiopeia's microservice architecture, and all need to be running simultaneously for the system to work. Viewer is Cassiopeia's client, and should be run last. If you have a terminal pane multiplexer you can use that, or you can simply open four terminal windows and navigate to Cassiopeia's /cmake/build directory in each. The executables can be started in any order, but ./viewer should be started last.

We highly recommend minimizing the Presenter, Registrar, and Gallery services to have the best experience.