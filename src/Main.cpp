#define VMA_IMPLEMENTATION

#include "Engine.hpp"

#include <iostream>

int main()
{
    std::unique_ptr<Engine> engine = std::make_unique<Engine>();

    return 0;
}
