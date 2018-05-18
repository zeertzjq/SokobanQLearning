#ifndef SokobanQLearning_SokobanQLearning_HPP_
#define SokobanQLearning_SokobanQLearning_HPP_ 1

#include "./Sokoban.hpp"
#include "./Utils.hpp"

#include <algorithm>
#include <array>
#include <bitset>
#include <cstddef>
#include <iomanip>
#include <ostream>
#include <random>
#include <unordered_map>

namespace SokobanQLearning {
    template <class RealType, std::size_t StateBits>
    class IQTable {
    public:
        typedef std::bitset<StateBits> StateType;

        virtual RealType Get(const StateType &state, const Sokoban::DirectionInt &action) const = 0;
        virtual void Set(const StateType &state, const Sokoban::DirectionInt &action, const RealType &value) = 0;
        virtual bool Check(const StateType &state) const = 0;
        virtual ~IQTable() = default;
    };

    template <class RealType, std::size_t StateBits>
    class QTable : public IQTable<RealType, StateBits> {
    public:
        typedef std::bitset<StateBits> StateType;

    protected:
        std::unordered_map<StateType, std::array<RealType, 4>> _map;

    public:
        RealType Get(const StateType &state, const Sokoban::DirectionInt &action) const override {
            if (!_map.count(state)) return 0;
            if (action == Sokoban::Up) return _map.at(state).at(0);
            if (action == Sokoban::Left) return _map.at(state).at(1);
            if (action == Sokoban::Right) return _map.at(state).at(2);
            if (action == Sokoban::Down) return _map.at(state).at(3);
            return 0;
        }

        void Set(const StateType &state, const Sokoban::DirectionInt &action, const RealType &value) override {
            if (action == Sokoban::Up) _map[state][0] = value;
            if (action == Sokoban::Left) _map[state][1] = value;
            if (action == Sokoban::Right) _map[state][2] = value;
            if (action == Sokoban::Down) _map[state][3] = value;
        }

        bool Check(const StateType &state) const override {
            return !!_map.count(state);
        }
    };

    template <class RealType, std::size_t StateBits>
    class PrintableQTable : public QTable<RealType, StateBits> {
    public:
        void Print(std::ostream &os, int precision, int column_width) const {
            os << std::right << std::setfill(' ') << std::fixed << std::setprecision(precision) << std::endl
               << std::setw(2 + (StateBits >> 2) + !!(StateBits & 0b11)) << "State" << std::setw(column_width) << "Up" << std::setw(column_width) << "Left" << std::setw(column_width) << "Right" << std::setw(column_width) << "Down" << std::endl;
            for (const auto &p : this->_map) {
                auto state_str = p.first.to_string();
                Utils::BinToHex(state_str);
                os << std::setw(5) << "0x" + state_str;
                for (const auto &value : p.second) os << std::setw(column_width) << value;
                os << std::endl;
            }
            os << std::endl;
        }
    };

    template <class URNG, class RealType, std::size_t StateBits>
    Sokoban::DirectionInt FindAction(URNG &random_generator, const double &epsilon, const Sokoban::Game<StateBits> &game, const IQTable<RealType, StateBits> &Q) {
        const auto &actions = game.GetDirections();
        if (!actions) return Sokoban::NoDirection;
        const auto &state = game.GetState();
        bool all_same = true;
        Sokoban::BitsInt action_count = 1;
        Sokoban::DirectionInt actions_remain = actions & (actions - 1);
        Sokoban::DirectionInt last_action = actions & -actions;
        Sokoban::DirectionInt current_action = actions_remain & -actions_remain;
        RealType last_Q = Q.Get(state, last_action);
        RealType current_Q = Q.Get(state, current_action);
        RealType max_Q = last_Q;
        Sokoban::DirectionInt choice = actions & -actions;
        while (actions_remain) {
            ++action_count;
            all_same = all_same && last_Q == current_Q;
            if (current_Q > max_Q) {
                max_Q = current_Q;
                choice = current_action;
            }
            actions_remain &= actions_remain - 1;
            last_action = current_action;
            last_Q = current_Q;
            current_action = actions_remain & -actions_remain;
            current_Q = Q.Get(state, current_action);
        }
        if (action_count == 1) return actions;
        const auto &random = std::uniform_real_distribution<double>(0.0, 1.0)(random_generator);
        if (all_same || random < epsilon) {
            Sokoban::BitsInt random_choice = std::uniform_int_distribution<int>(0, action_count - 1)(random_generator);
            actions_remain = actions;
            while (random_choice--) actions_remain &= actions_remain - 1;
            return actions_remain & -actions_remain;
        } else
            return choice;
    }

    template <class URNG, class RealType, std::size_t StateBits>
    Sokoban::DirectionInt Train(URNG &random_generator, Sokoban::Game<StateBits> &game, IQTable<RealType, StateBits> &Q, const double &epsilon, const RealType &alpha, const RealType &gamma, const RealType &retrace_penalty, const RealType &push_reward, const RealType &goal_reward, const RealType &failure_penalty, const RealType &success_reward) {
        if (game.GetSucceeded() || game.GetFailed()) {
            game.Restart();
            return Sokoban::NoDirection;
        }
        const auto last_state = game.GetState();
        const auto last_finished = game.GetFinished();
        const auto &last_action = FindAction(random_generator, epsilon, game, Q);
        const bool &pushed = game.Move(last_action);
        RealType reward = goal_reward * (game.GetFinished() - last_finished);
        if (game.GetStateHistory().count(game.GetState())) reward -= retrace_penalty;
        if (pushed) reward += push_reward;
        if (game.GetSucceeded()) reward += success_reward;
        if (game.GetFailed()) reward -= failure_penalty;
        const auto &actions = game.GetDirections();
        const auto &state = game.GetState();
        RealType max_Q = -(retrace_penalty + failure_penalty + goal_reward * game.GetBoxPos0().size());
        for (const auto &d : {Sokoban::Up, Sokoban::Left, Sokoban::Right, Sokoban::Down})
            if (actions & d) max_Q = std::max(max_Q, Q.Get(state, d));
        Q.Set(last_state, last_action, (static_cast<RealType>(1) - alpha) * Q.Get(last_state, last_action) + alpha * (reward + gamma * max_Q));
        return last_action;
    }
}  // namespace SokobanQLearning

#endif  // SokobanQLearning_SokobanQLearning_HPP_
