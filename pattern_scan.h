#pragma once

#include <cstddef>
#include <cstdint>
#include <cctype>
#include <charconv>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

inline std::pair<std::vector<uint8_t>, std::string> ParsePattern(std::string_view text)
{
    std::vector<uint8_t> bytes;
    std::string mask;
    while (!text.empty())
    {
        if (std::isspace(static_cast<unsigned char>(text.front())))
        {
            text.remove_prefix(1);
            continue;
        }

        size_t length = text.find_first_of(" \t\r\n\v\f");
        if (length == std::string_view::npos)
            length = text.size();
        const auto token = text.substr(0, length);
        text.remove_prefix(length);
        if (token == "?" || token == "??")
        {
            bytes.push_back(0);
            mask += '?';
            continue;
        }

        unsigned int byte = 0;
        const char* end = token.data() + token.size();
        const auto value = std::from_chars(token.data(), end, byte, 16);
        if (value.ec != std::errc{} || value.ptr != end || byte > 255 || token.size() > 2)
            return {};
        bytes.push_back(static_cast<uint8_t>(byte));
        mask += 'x';
    }
    return {std::move(bytes), std::move(mask)};
}

inline const uint8_t* ScanPattern(const uint8_t* data, size_t size, const uint8_t* pattern, std::string_view mask)
{
    if (!data || !pattern || mask.empty() || mask.size() > size)
        return nullptr;

    size_t anchor = mask.find('x');
    if (anchor == std::string_view::npos)
        return data;

    size_t candidates = size - mask.size() + 1;
    while (candidates)
    {
        const auto* hit = static_cast<const uint8_t*>(std::memchr(data + anchor, pattern[anchor], candidates));
        if (!hit)
            return nullptr;

        const auto* start = hit - anchor;
        size_t index = anchor + 1;
        for (; index < mask.size(); ++index)
        {
            if (mask[index] == 'x' && start[index] != pattern[index])
                break;
        }
        if (index == mask.size())
            return start;

        candidates -= static_cast<size_t>(start - data) + 1;
        data = start + 1;
    }
    return nullptr;
}
