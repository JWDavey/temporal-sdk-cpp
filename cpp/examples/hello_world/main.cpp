/// @file hello_world/main.cpp
/// @brief Minimal example: start a workflow and get the result.

#include <temporalio/version.h>

#include <iostream>

int main() {
    std::cout << "Temporal C++ SDK v" << temporalio::version() << "\n";
    std::cout << "Hello World example - not yet implemented\n";
    return 0;
}
