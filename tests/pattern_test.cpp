#include "pattern_scan.h"

#include <cassert>
#include <iostream>

int main()
{
    const uint8_t data[] = {0x55, 0x01, 0x55, 0x02, 0x91};
    auto [pattern, mask] = ParsePattern("55 ? 91");
    assert(ScanPattern(data, sizeof(data), pattern.data(), mask) == data + 2);
    assert(ScanPattern(data, mask.size() - 1, pattern.data(), mask) == nullptr);
    assert(ScanPattern(data + 2, mask.size(), pattern.data(), mask) == data + 2);
    assert(ScanPattern(data, sizeof(data), pattern.data(), "") == nullptr);

    auto [wildcard, wildcardMask] = ParsePattern("\t?? 02\n");
    assert(ScanPattern(data, sizeof(data), wildcard.data(), wildcardMask) == data + 2);
    assert(ParsePattern("GG").first.empty());
    assert(ParsePattern("001").first.empty());
    assert(ParsePattern(std::string_view("AA ZZ", 2)).second == "x");

    const uint8_t last = 0x91;
    assert(ScanPattern(data, sizeof(data), &last, "x") == data + 4);
    assert(ScanPattern(&last, 1, &last, "x") == &last);
    std::cout << "Signature scanner tests passed\n";
}
