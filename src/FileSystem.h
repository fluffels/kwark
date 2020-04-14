#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "easylogging++.h"

std::vector<char> readFile(const std::filesystem::path&);
