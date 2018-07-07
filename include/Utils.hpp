#ifndef SokobanQLearning_Utils_HPP_
#define SokobanQLearning_Utils_HPP_ 1

#include <bitset>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>

namespace Utils {
    template <std::size_t N>
    std::string BitsToHex(const std::bitset<N> &);

#ifdef SokobanQLearning_USE_EMOJI_
    std::string MazeToEmoji(const std::string &);
#endif
}  // namespace Utils
template <std::size_t N>
std::string Utils::BitsToHex(const std::bitset<N> &bits) {
    auto str = bits.to_string();
    while (str.size() & 0b11) str = '0' + str;
    std::ostringstream oss;
    for (std::string::size_type i = 0; i < str.size(); i += 4)
        oss << std::hex << std::bitset<4>(str, i, 4).to_ulong();
    return oss.str();
}

#ifdef SokobanQLearning_USE_EMOJI_
std::string Utils::MazeToEmoji(const std::string &maze) {
    std::string ret;
    for (const auto &c : maze) {
        switch (c) {
            case '*':
            case '+':
                ret += "\U0001f643";
                break;
            case '&':
            case '@':
                ret += "\U0001f4e6";
                break;
            case '$':
                ret += "\u2b55";
                break;
            case '.':
                ret += "\u2b1b";
                break;
            case '#':
                ret += "\u2b1c";
                break;
            default:
                ret += c;
        }
    }
    return ret;
}
#endif

#endif  // SokobanQLearning_Utils_HPP_
