#define VMA_IMPLEMENTATION

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "Engine.hpp"

#include <iostream>

int main()
{
    std::unique_ptr<Engine> engine = std::make_unique<Engine>();

    return 0;
}
