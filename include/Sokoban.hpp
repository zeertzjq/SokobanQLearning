#ifndef SokobanQLearning_Sokoban_HPP_
#define SokobanQLearning_Sokoban_HPP_ 1

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Sokoban {
    class Error : public std::runtime_error {
    public:
        explicit Error(const std::string &message) : std::runtime_error(message) {}
    };

    typedef std::uint_least8_t DirectionInt;
    constexpr DirectionInt NoDirection = 0b0000;
    constexpr DirectionInt Up = 0b0001;
    constexpr DirectionInt Left = 0b0010;
    constexpr DirectionInt Right = 0b0100;
    constexpr DirectionInt Down = 0b1000;

    typedef std::uint_least8_t PosInt;
    constexpr PosInt IsWall = 0b0000;
    constexpr PosInt IsFloor = 0b0001;
    constexpr PosInt IsGoal = 0b0010;
    constexpr PosInt IsBox = 0b0100;
    constexpr PosInt IsPlayer = 0b1000;

    typedef std::int_least8_t BitsInt;
    typedef std::int_least8_t MazeInt;
    typedef std::int_least16_t SizeInt;
    typedef std::uint_least64_t TimeInt;
    typedef std::pair<MazeInt, MazeInt> Pos;

    constexpr Pos Movement(const DirectionInt &direction);

    template <std::size_t StateBits>
    class Game {
    public:
        typedef std::bitset<StateBits> StateType;

    private:
        bool Succeeded, Failed;
        DirectionInt Directions;
        MazeInt Height, Width;
        BitsInt FloorBits;
        SizeInt Finished;
        StateType State;
        TimeInt TimeElapsed;
        Pos PlayerPos0, PlayerPos;
        std::set<Pos> BoxPos0, BoxPos, GoalPos;
        std::vector<std::vector<PosInt>> Maze;
        std::vector<std::vector<SizeInt>> FloorIndex;
        std::unordered_set<StateType> StateHistory;

        void UpdateData() {
            Maze.clear();
            Maze.resize(Height, std::vector<PosInt>(Width, IsWall));
            Finished = 0;
            for (MazeInt i = 0; i < Height; ++i)
                for (MazeInt j = 0; j < Width; ++j)
                    if (FloorIndex[i][j] >= 0) Maze[i][j] = IsFloor;
            for (const auto &i : GoalPos)
                Maze[i.first][i.second] |= IsGoal;
            State = FloorIndex[PlayerPos.first][PlayerPos.second];
            SizeInt offset = 0;
            for (const auto &i : BoxPos) {
                Maze[i.first][i.second] |= IsBox;
                if (Maze[i.first][i.second] == (IsFloor | IsGoal | IsBox)) ++Finished;
                State |= StateType(FloorIndex[i.first][i.second]) << FloorBits * ++offset;
            }
            Maze[PlayerPos.first][PlayerPos.second] |= IsPlayer;
            Directions = NoDirection;
            for (const auto &d : {Up, Left, Right, Down})
                if (CheckDirection(d, PlayerPos.first, PlayerPos.second, true)) Directions |= d;
            Succeeded = Finished == BoxPos0.size();
            Failed = CheckFailed();
        }

        bool CheckFloor(const MazeInt &line, const MazeInt &col) const {
            if (line < 0 || line >= Height) return false;
            if (col < 0 || col >= Width) return false;
            return FloorIndex[line][col] >= 0;
        }

        bool CheckDirection(const DirectionInt &direction, const MazeInt &line, const MazeInt &col, const bool &can_push_box) const {
            const auto &movement = Movement(direction);
            if (!CheckFloor(line + movement.first, col + movement.second)) return false;
            if (Maze[line + movement.first][col + movement.second] & IsBox) {
                if (can_push_box) {
                    if (!CheckDirection(direction, line + movement.first, col + movement.second, false))
                        return false;
                } else
                    return false;
            }
            return true;
        }

        bool BoxStuck(const MazeInt &line, const MazeInt &col, const bool &is_horizontal, std::set<std::pair<Pos, bool>> &vis) const {
            const auto &current = std::make_pair(Pos{line, col}, is_horizontal);
            if (vis.count(current)) {
                vis.erase(current);
                return true;
            }
            vis.insert(current);
            bool ret = false;
            if (is_horizontal) {
                if (!CheckFloor(line, col - 1) || !CheckFloor(line, col + 1))
                    ret = true;
                else if (Maze[line][col - 1] & IsBox && BoxStuck(line, col - 1, !is_horizontal, vis))
                    ret = true;
                else if (Maze[line][col + 1] & IsBox && BoxStuck(line, col + 1, !is_horizontal, vis))
                    ret = true;
            } else {
                if (!CheckFloor(line - 1, col) || !CheckFloor(line + 1, col))
                    ret = true;
                else if (Maze[line - 1][col] & IsBox && BoxStuck(line - 1, col, !is_horizontal, vis))
                    ret = true;
                else if (Maze[line + 1][col] & IsBox && BoxStuck(line + 1, col, !is_horizontal, vis))
                    ret = true;
            }
            vis.erase(current);
            return ret;
        }

        bool WallStuck(const MazeInt &line, const MazeInt &col, const DirectionInt &side) const {
            MazeInt box_count = !!(Maze[line][col] & IsBox);
            MazeInt goal_count = !!(Maze[line][col] & IsGoal);
            const auto &movement = Movement(side);
            if (movement.first) {
                for (MazeInt p = col - 1; CheckFloor(line, p); --p) {
                    if (CheckFloor(line - movement.first, p)) return false;
                    if (Maze[line][p] & IsBox) ++box_count;
                    if (Maze[line][p] & IsGoal) ++goal_count;
                }
                for (MazeInt p = col + 1; CheckFloor(line, p); ++p) {
                    if (CheckFloor(line - movement.first, p)) return false;
                    if (Maze[line][p] & IsBox) ++box_count;
                    if (Maze[line][p] & IsGoal) ++goal_count;
                }
            } else {
                for (MazeInt p = line - 1; CheckFloor(p, col); --p) {
                    if (CheckFloor(p, col - movement.second)) return false;
                    if (Maze[p][col] & IsBox) ++box_count;
                    if (Maze[p][col] & IsGoal) ++goal_count;
                }
                for (MazeInt p = line + 1; CheckFloor(p, col); ++p) {
                    if (CheckFloor(p, col - movement.second)) return false;
                    if (Maze[p][col] & IsBox) ++box_count;
                    if (Maze[p][col] & IsGoal) ++goal_count;
                }
            }
            return box_count > goal_count;
        }

        bool CanPushAny(const MazeInt &line, const MazeInt &col, std::set<Pos> &vis) const {
            const Pos p{line, col};
            if (vis.count(p)) return false;
            vis.insert(p);
            bool ret = false;
            for (const auto &d : {Up, Left, Right, Down}) {
                const auto &movement = Movement(d);
                if (CheckDirection(d, line, col, false))
                    ret = ret || CanPushAny(line + movement.first, col + movement.second, vis);
                else
                    ret = ret || CheckDirection(d, line, col, true);
            }
            return ret;
        }

        bool CheckFailed() const {
            if (Finished == BoxPos0.size()) return false;
            if (!Directions) return true;
            for (const auto &b : BoxPos) {
                std::set<std::pair<Pos, bool>> vis;
                vis.clear();
                const bool &stuck_vertical = BoxStuck(b.first, b.second, false, vis);
                vis.clear();
                const bool &stuck_horizontal = BoxStuck(b.first, b.second, true, vis);
                if (Maze[b.first][b.second] != (IsFloor | IsGoal | IsBox)) {
                    if (stuck_vertical && stuck_horizontal) return true;
                    if (stuck_vertical) {
                        bool stuck = false;
                        if (!CheckFloor(b.first - 1, b.second)) stuck = stuck || WallStuck(b.first, b.second, Down);
                        if (!CheckFloor(b.first + 1, b.second)) stuck = stuck || WallStuck(b.first, b.second, Up);
                        if (stuck) return true;
                    }
                    if (stuck_horizontal) {
                        bool stuck = false;
                        if (!CheckFloor(b.first, b.second - 1)) stuck = stuck || WallStuck(b.first, b.second, Right);
                        if (!CheckFloor(b.first, b.second + 1)) stuck = stuck || WallStuck(b.first, b.second, Left);
                        if (stuck) return true;
                    }
                }
            }
            std::set<Pos> vis;
            if (!CanPushAny(PlayerPos.first, PlayerPos.second, vis)) return true;
            return false;
        }

        void DoRestart() {
            TimeElapsed = 0;
            StateHistory.clear();
            PlayerPos = PlayerPos0;
            BoxPos = BoxPos0;
            UpdateData();
        }

        bool DoMove(const DirectionInt &direction) {
            if (!(Directions & direction)) return false;
            const auto &movement = Movement(direction);
            if (movement.first == movement.second) return false;
            ++TimeElapsed;
            StateHistory.insert(State);
            PlayerPos = {PlayerPos.first + movement.first, PlayerPos.second + movement.second};
            const auto &pushed_box = BoxPos.find(PlayerPos);
            bool pushed = false;
            if (pushed_box != BoxPos.end()) {
                Pos p{pushed_box->first + movement.first, pushed_box->second + movement.second};
                BoxPos.erase(pushed_box);
                BoxPos.insert(p);
                pushed = true;
            }
            UpdateData();
            return pushed;
        }

        void WriteMazeString(std::string &str) const {
            str.clear();
            for (const auto &row : Maze) {
                for (const auto &c : row) {
                    switch (c) {
                        case IsWall:
                            str += '#';
                            break;
                        case IsFloor:
                            str += '.';
                            break;
                        case IsFloor | IsGoal:
                            str += '$';
                            break;
                        case IsFloor | IsBox:
                            str += '&';
                            break;
                        case IsFloor | IsGoal | IsBox:
                            str += '@';
                            break;
                        case IsFloor | IsPlayer:
                            str += '*';
                            break;
                        case IsFloor | IsGoal | IsPlayer:
                            str += '+';
                            break;
                    }
                }
                str += '\n';
            }
            str.pop_back();
        }

    public:
        const auto &GetSucceeded() const {
            return Succeeded;
        }

        const auto &GetFailed() const {
            return Failed;
        }

        const auto &GetDirections() const {
            return Directions;
        }

        const auto &GetHeight() const {
            return Height;
        }

        const auto &GetWidth() const {
            return Width;
        }

        const auto &GetFloorBits() const {
            return FloorBits;
        }

        const auto &GetFinished() const {
            return Finished;
        }

        const auto &GetState() const {
            return State;
        }

        const auto &GetTimeElapsed() const {
            return TimeElapsed;
        }

        const auto &GetPlayerPos0() const {
            return PlayerPos0;
        }

        const auto &GetPlayerPos() const {
            return PlayerPos;
        }

        const auto &GetBoxPos0() const {
            return BoxPos0;
        }

        const auto &GetBoxPos() const {
            return BoxPos;
        }

        const auto &GetGoalPos() const {
            return GoalPos;
        }

        const auto &GetMaze() const {
            return Maze;
        }

        const auto &GetFloorIndex() const {
            return FloorIndex;
        }

        const auto &GetStateHistory() const {
            return StateHistory;
        }

        void GetMazeString(std::string &str) const {
            WriteMazeString(str);
        }

        void Restart() {
            DoRestart();
        }

        bool Move(const DirectionInt &direction) {
            return DoMove(direction);
        }

        Game(std::string maze) {
            while (maze.size() && maze.front() == '\n')
                maze.erase(maze.begin());
            while (maze.size() && maze.back() == '\n')
                maze.pop_back();
            std::queue<Pos> floor;
            PlayerPos0 = {-1, -1};
            MazeInt line = 0;
            MazeInt col = -1;
            Height = 1;
            Width = 0;
            for (const auto &c : maze) {
                switch (c) {
                    case '\r':
                        break;
                    case '\n':
                        ++line;
                        col = -1;
                        if (line >= 125) throw Error("Maze Too Large");
                        if (line >= Height) Height = line + 1;
                        break;
                    default:
                        ++col;
                        if (col >= 125) throw Error("Maze Too Large");
                        if (col >= Width) Width = col + 1;
                        switch (c) {
                            case '.':
                                floor.emplace(line, col);
                                break;
                            case '*':
                                floor.emplace(line, col);
                                if (PlayerPos0.first + PlayerPos0.second >= 0) throw Error("Too Many Players");
                                PlayerPos0 = {line, col};
                                break;
                            case '$':
                                floor.emplace(line, col);
                                GoalPos.emplace(line, col);
                                break;
                            case '&':
                                floor.emplace(line, col);
                                BoxPos0.emplace(line, col);
                                break;
                            case '+':
                                floor.emplace(line, col);
                                if (PlayerPos0.first + PlayerPos0.second >= 0) throw Error("Too Many Players");
                                PlayerPos0 = {line, col};
                                GoalPos.emplace(line, col);
                                break;
                            case '@':
                                floor.emplace(line, col);
                                BoxPos0.emplace(line, col);
                                GoalPos.emplace(line, col);
                                break;
                        }
                        break;
                }
            }
            if (PlayerPos0.first + PlayerPos0.second == -2) throw Error("No Player");
            if (BoxPos0.empty()) throw Error("No Box");
            if (BoxPos0.size() > GoalPos.size()) throw Error("Too Few Goals");
            FloorBits = 0;
            for (SizeInt floor_remain = floor.size() - 1; floor_remain; floor_remain >>= 1) ++FloorBits;
            if (FloorBits * (BoxPos0.size() + 1) > StateBits) throw Error("Maze Too Large");
            FloorIndex.resize(Height, std::vector<SizeInt>(Width, -1));
            SizeInt index = -1;
            while (!floor.empty()) {
                const auto &p = floor.front();
                FloorIndex[p.first][p.second] = ++index;
                floor.pop();
            }
            DoRestart();
        }
    };
}  // namespace Sokoban

constexpr Sokoban::Pos Sokoban::Movement(const Sokoban::DirectionInt &direction) {
    if (direction == Up) return {-1, 0};
    if (direction == Left) return {0, -1};
    if (direction == Right) return {0, 1};
    if (direction == Down) return {1, 0};
    return {0, 0};
}

#endif  // SokobanQLearning_Sokoban_HPP_
