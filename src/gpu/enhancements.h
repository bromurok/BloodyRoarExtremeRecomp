#pragma once
#include <cstdint>

namespace enhancements {

struct Config {
    bool anti_aliasing = true;   // Suavizado de bordes (Anti-Aliasing)
};

Config& Get();
void Load();
void Save();

}  // namespace enhancements
