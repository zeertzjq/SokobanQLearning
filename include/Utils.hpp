#ifndef SokobanQLearning_Utils_HPP_
#define SokobanQLearning_Utils_HPP_ 1

#include <iomanip>
#include <sstream>
#include <string>

namespace Utils {
    void BinToHex(std::string &);

#ifdef SokobanQLearning_USE_EMOJI_
    void MazeToEmoji(std::string &);
#endif
}  // namespace Utils

void Utils::BinToHex(std::string &str) {
    while (str.size() & 0b11) str = '0' + str;
    std::ostringstream oss;
    for (std::string::size_type i = 0; i < str.size(); i += 4)
        oss << std::hex << std::bitset<4>(str, i, 4).to_ulong();
    str = oss.str();
}

#ifdef SokobanQLearning_USE_EMOJI_
void Utils::MazeToEmoji(std::string &maze) {
    const auto old_maze = maze;
    maze.clear();
    for (const auto &c : old_maze) {
        switch (c) {
            case '*':
            case '+':
                maze += "\U0001f643";
                break;
            case '&':
            case '@':
                maze += "\U0001f4e6";
                break;
            case '$':
                maze += "\u2b55";
                break;
            case '.':
                maze += "\u2b1b";
                break;
            case '#':
                maze += "\u2b1c";
                break;
            default:
                maze += c;
                break;
        }
    }
}
#endif

#endif  // SokobanQLearning_Utils_HPP_
