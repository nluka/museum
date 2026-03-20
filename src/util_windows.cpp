#include "util_windows.hpp"

#include <algorithm>

#include <Shlwapi.h>

winapi_error get_last_winapi_error() noexcept
{
    DWORD error_code = GetLastError();
    if (error_code == 0) return {0, "No error."};

    LPSTR buffer = nullptr;
    DWORD buffer_size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&buffer), 0, nullptr
    );

    std::string msg;
    if (buffer_size != 0)
    {
        msg.assign(buffer, buffer + buffer_size);
        LocalFree(buffer);
        while (!msg.empty() && (msg.back() == '\r' || msg.back() == '\n')) msg.pop_back();
    }
    else msg = "Error formatting message.";

    return {error_code, msg};
}

std::pair<s32, std::array<char, 64>> filetime_to_string(FILETIME *time) noexcept
{
    std::array<char, 64> buffer_raw;
    std::array<char, 64> buffer_final;

    DWORD flags = FDTF_SHORTDATE | FDTF_SHORTTIME;
    s32 length = SHFormatDateTimeA(time, &flags, buffer_raw.data(), (u32)buffer_raw.size());

    // for some reason, SHFormatDateTimeA will pad parts of the string with ASCII 63 (?)
    // when using LONGDATE or LONGTIME, we are discarding them
    std::copy_if(buffer_raw.begin(), buffer_raw.end(), buffer_final.begin(), [](char ch) noexcept { return ch != '?'; });

    // std::replace(buffer_final.begin(), buffer_final.end(), '-', ' ');

    return { length, buffer_final };
}
