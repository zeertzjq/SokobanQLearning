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
        typedef typename Sokoban::Game<StateBits>::StateType StateType;
        typedef std::array<RealType, 4> RowType;

        virtual RealType Get(const StateType &state, const Sokoban::DirectionInt &action) const = 0;
        virtual RowType Get(const StateType &state) const = 0;
        virtual void Set(const StateType &state, const Sokoban::DirectionInt &action, const RealType &value) = 0;
        virtual void Set(const StateType &state, const RowType &row) = 0;
        virtual bool Check(const StateType &state) const = 0;
        virtual ~IQTable() = default;
    };

    template <class RealType, std::size_t StateBits>
    class QTable : public IQTable<RealType, StateBits> {
    public:
        using typename IQTable<RealType, StateBits>::StateType;
        using typename IQTable<RealType, StateBits>::RowType;

    protected:
        std::unordered_map<StateType, std::array<RealType, 4>> _map;

    public:
        RealType Get(const StateType &state, const Sokoban::DirectionInt &action) const override {
            if (!_map.count(state)) return 0;
            switch (action) {
                case Sokoban::Up:
                    return _map.at(state).at(0);
                case Sokoban::Left:
                    return _map.at(state).at(1);
                case Sokoban::Right:
                    return _map.at(state).at(2);
                case Sokoban::Down:
                    return _map.at(state).at(3);
                default:
                    return 0;
            }
        }

        RowType Get(const StateType &state) const override {
            if (_map.count(state))
                return _map.at(state);
            else
                return {{0, 0, 0, 0}};
        }

        void Set(const StateType &state, const Sokoban::DirectionInt &action, const RealType &value) override {
            switch (action) {
                case Sokoban::Up:
                    _map[state][0] = value;
                    return;
                case Sokoban::Left:
                    _map[state][1] = value;
                    return;
                case Sokoban::Right:
                    _map[state][2] = value;
                    return;
                case Sokoban::Down:
                    _map[state][3] = value;
                    return;
            }
        }

        void Set(const StateType &state, const RowType &row) override {
            _map[state] = row;
        }

        bool Check(const StateType &state) const override {
            return _map.count(state);
        }
    };

    template <class RealType, std::size_t StateBits>
    class PrintableQTable : public QTable<RealType, StateBits> {
    public:
        typedef typename IQTable<RealType, StateBits>::StateType StateType;
        static constexpr std::size_t FirstColumnWidth = std::max(2 + (StateBits >> 2) + !!(StateBits & 0b11), static_cast<std::size_t>(6));

        void PrintStateRow(std::ostream &os, int precision, int column_width, const StateType &state) const {
            os << std::right << std::setfill(' ') << std::setw(FirstColumnWidth) << "0x" + Utils::BinToHex(std::move(state.to_string())) << std::setprecision(precision) << std::fixed;
            for (const auto &value : this->Get(state))
                os << std::setw(column_width) << value;
            os << std::endl;
        }

        void PrintHeader(std::ostream &os, int column_width) const {
            os << std::right << std::setfill(' ') << std::setw(FirstColumnWidth) << "State";
            for (const auto &d : Sokoban::AllDirections)
                os << std::setw(column_width) << Sokoban::DirectionName(d);
            os << std::endl;
        }

        void Print(std::ostream &os, int precision, int column_width) const {
            os << std::string(FirstColumnWidth + 4 * column_width, '=') << std::endl;
            PrintHeader(os, column_width);
            for (const auto &p : this->_map) PrintStateRow(os, precision, column_width, p.first);
            os << std::endl;
        }
    };

    template <class RealType, std::size_t StateBits>
    struct TrainResult {
    public:
        typedef typename IQTable<RealType, StateBits>::StateType StateType;
        typedef typename IQTable<RealType, StateBits>::RowType RowType;
        static constexpr std::size_t FirstColumnWidth = std::max(2 + (StateBits >> 2) + !!(StateBits & 0b11), static_cast<std::size_t>(6));

        Sokoban::DirectionInt Action;
        StateType LastState;
        RowType OldRow, NewRow;

        void Print(std::ostream &os, int precision, int column_width) const {
            os << "Last State: 0x" << Utils::BinToHex(std::move(LastState.to_string())) << std::endl;
            os << "Action: " << Sokoban::DirectionName(Action) << std::endl;
            if (Action == Sokoban::NoDirection) return;
            os << std::endl
               << std::right << std::setfill(' ') << std::setw(FirstColumnWidth) << "0x" + Utils::BinToHex(std::move(LastState.to_string()));
            for (const auto &d : Sokoban::AllDirections)
                os << std::setw(column_width) << Sokoban::DirectionName(d);
            os << std::endl
               << std::fixed << std::setprecision(precision);
            os << std::setw(FirstColumnWidth) << "Old";
            for (const auto &value : OldRow)
                os << std::setw(column_width) << value;
            os << std::endl;
            os << std::setw(FirstColumnWidth) << "New";
            for (const auto &value : NewRow)
                os << std::setw(column_width) << value;
            os << std::endl
               << std::endl;
        }

        TrainResult(const StateType &last_state, const RowType &old_row, const Sokoban::DirectionInt &action, const RowType &new_row) : Action(action), LastState(last_state), OldRow(old_row), NewRow(new_row) {}
        TrainResult(const StateType &last_state, const RowType &old_row) : TrainResult(last_state, old_row, Sokoban::NoDirection, old_row) {}
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
    TrainResult<RealType, StateBits> Train(URNG &random_generator, Sokoban::Game<StateBits> &game, IQTable<RealType, StateBits> &Q, const double &epsilon, const RealType &alpha, const RealType &gamma, const RealType &retrace_penalty, const RealType &push_reward, const RealType &goal_reward, const RealType &failure_penalty, const RealType &success_reward) {
        const auto last_state = game.GetState();
        const auto old_row = Q.Get(last_state);
        if (game.GetSucceeded() || game.GetFailed()) {
            game.Restart();
            return {last_state, old_row};
        }
        const auto last_finished = game.GetFinished();
        const auto last_action = FindAction(random_generator, epsilon, game, Q);
        const bool pushed = game.Move(last_action);
        const auto state = game.GetState();
        RealType reward = goal_reward * (game.GetFinished() - last_finished);
        if (game.GetStateHistory().count(state)) reward -= retrace_penalty;
        if (pushed) reward += push_reward;
        if (game.GetSucceeded()) reward += success_reward;
        if (game.GetFailed()) reward -= failure_penalty;
        const auto actions = game.GetDirections();
        RealType max_Q = -(retrace_penalty + failure_penalty + goal_reward * game.GetBoxPos0().size());
        for (const auto &d : Sokoban::AllDirections)
            if (actions & d)
                max_Q = std::max(max_Q, Q.Get(state, d));
        Q.Set(last_state, last_action, (static_cast<RealType>(1) - alpha) * Q.Get(last_state, last_action) + alpha * (reward + gamma * max_Q));
        return {last_state, old_row, last_action, Q.Get(last_state)};
    }
}  // namespace SokobanQLearning

#endif  // SokobanQLearning_SokobanQLearning_HPP_
