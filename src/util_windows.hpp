#pragma once

#include <utility>
#include <array>
#include <string>

#include <Windows.h>

#include "primitives.hpp"

struct winapi_error
{
    DWORD code;
    std::string formatted_message;
};
winapi_error get_last_winapi_error() noexcept;

std::pair<s32, std::array<char, 64>> filetime_to_string(FILETIME *time) noexcept;
