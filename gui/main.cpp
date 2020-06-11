#include "stmdsp.hpp"

#include <iostream>

int main()
{
    stmdsp::scanner scanner;

    scanner.scan();
    for (const auto& device : scanner.devices())
        std::cout << "Found stmdsp at: " << device.path() << std::endl;

    return 0;
}

