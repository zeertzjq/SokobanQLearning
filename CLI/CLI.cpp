#include "../include/Sokoban.hpp"
#include "../include/SokobanQLearning.hpp"
#include "../include/Utils.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#undef SokobanQLearning_CLI_USE_WINAPI_

#if defined(_WIN32) && !defined(SokobanQLearning_CLI_NO_WINAPI_)
#define SokobanQLearning_CLI_USE_WINAPI_
#include <windows.h>
#endif

namespace {
    void PrintOption(std::ostream &os, const std::string &option, const std::string &description) {
        os << std::string(4, ' ') << std::left << std::setfill(' ');
        if (option.size() <= 18)
            os << std::setw(20) << option << description << std::endl;
        else
            os << option << std::endl
               << std::string(24, ' ') << description << std::endl;
    }

    bool print_Q_success = false;
    bool print_Q_failure = false;
    bool print_Q_exit = false;
    long long sleep = 100;
    long long quiet = 0;
    bool random_device = false;
    std::atomic_bool interrupted;

#ifdef SokobanQLearning_CLI_USE_WINAPI_
    bool ansi_escape = false;

    void ClearConsoleWin() {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD coordScreen = {0, 0};
        DWORD cCharsWritten;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        DWORD dwConSize;
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
        FillConsoleOutputCharacter(hConsole, ' ', dwConSize, coordScreen, &cCharsWritten);
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
        SetConsoleCursorPosition(hConsole, coordScreen);
    }
#endif

#ifdef SokobanQLearning_USE_EMOJI_
    bool emoji = false;
#endif

    void ClearConsole() {
#ifdef SokobanQLearning_CLI_USE_WINAPI_
        if (!ansi_escape) {
            ClearConsoleWin();
            return;
        }
#endif
        std::cout << "\x1b[H\x1b[2J" << std::flush;
    }

    template <typename RealType, std::size_t StateBits>
    bool RunAlgorithm(std::string maze) {
        std::shared_ptr<Sokoban::Game<StateBits>> game_ptr;
        try {
            game_ptr = std::make_shared<Sokoban::Game<StateBits>>(std::move(maze));
            maze.clear();
        } catch (const Sokoban::Error &err) {
            std::cerr << "Error: " << err.what() << std::endl;
            return false;
        }
        auto &game = *game_ptr;
        SokobanQLearning::PrintableQTable<RealType, StateBits> Q;
        std::mt19937 random_engine(random_device ? std::random_device()() : std::chrono::system_clock::now().time_since_epoch().count());
        interrupted = false;
        std::signal(SIGINT, [](int) -> void {
            interrupted = true;
        });
        auto train = std::bind(SokobanQLearning::Train<decltype(random_engine), RealType, StateBits>, std::ref(random_engine), std::ref(game), std::ref(Q), 0.05, 0.5f, 1.0f, 1.0f, 0.5f, 50.0f, 1000.0f, 1000.0f);
        while (!interrupted && quiet-- > 1) train();
        if (interrupted) {
            std::cout << std::endl;
            if (print_Q_exit) Q.Print(std::clog, 4, 12);
            return true;
        }
        SokobanQLearning::TrainResult<RealType, StateBits> train_result = quiet >= 0 ? train() : decltype(train_result){game.GetState(), Q.Get(game.GetState())};
        while (!interrupted) {
            ClearConsole();
            maze = game.GetMazeString();
#ifdef SokobanQLearning_USE_EMOJI_
            if (emoji) maze = Utils::MazeToEmoji(maze);
#endif
            std::cout << std::endl
                      << maze << std::endl
                      << std::endl
                      << "Time: " << std::dec << game.GetTimeElapsed() << std::endl
                      << "State: 0x" << Utils::BitsToHex(game.GetState()) << std::endl;
            std::cout << std::endl;
            Q.PrintHeader(std::cout, 12);
            Q.PrintStateRow(std::cout, 4, 12, game.GetState());
            std::cout << std::endl;
            train_result.Print(std::cout, 4, 12);
            if (game.GetSucceeded()) {
#ifdef SokobanQLearning_USE_EMOJI_
                if (emoji) std::cout << "\U00002b55";
#endif
                std::cout << "Succeeded" << std::endl;
                if (print_Q_success) {
                    std::clog << std::endl;
                    Q.Print(std::clog, 4, 12);
                }
            } else if (game.GetFailed()) {
#ifdef SokobanQLearning_USE_EMOJI_
                if (emoji) std::cout << "\U0000274c";
#endif
                std::cout << "Failed" << std::endl;
                if (print_Q_failure) {
                    std::clog << std::endl;
                    Q.Print(std::clog, 4, 12);
                }
            }
            if (sleep) std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
            train_result = train();
        }
        if (print_Q_exit) {
            std::clog << std::endl;
            Q.Print(std::clog, 4, 12);
        }
        return true;
    }
}  // namespace

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl
                      << std::endl
                      << "Options:" << std::endl;
            PrintOption(std::cout, "--help", "Print this help message");
            PrintOption(std::cout, "--print-q", "Print the Q table on success, failure and exit");
            PrintOption(std::cout, "--print-q-success", "Print the Q table on success");
            PrintOption(std::cout, "--print-q-failure", "Print the Q table on failure");
            PrintOption(std::cout, "--print-q-exit", "Print the Q table on exit");
            PrintOption(std::cout, "--sleep=<num>", "Sleep for <num> milliseconds between two steps (default value is 100)");
            PrintOption(std::cout, "--quiet=<num>", "Train for <num> steps before doing anything else (default value is 0)");
            PrintOption(std::cout, "--random-device", "Obtain the random seed from the system random device instead of the system time (NOT GUARANTEED TO WORK)");
#ifdef SokobanQLearning_USE_EMOJI_
            PrintOption(std::cout, "--emoji", "Using emoji symbols in output (NOT GUARANTEED TO WORK)");
#endif
#ifdef SokobanQLearning_CLI_USE_WINAPI_
            PrintOption(std::cout, "--ansi-escape", "Clear the console using ansi escape sequences instead of calling Windows APIs");
#endif
            std::cout << std::endl;
            return 0;
        } else if (arg == "--print-q") {
            print_Q_success = print_Q_failure = print_Q_exit = true;
        } else if (arg == "--print-q-success") {
            print_Q_success = true;
        } else if (arg == "--print-q-failure") {
            print_Q_failure = true;
        } else if (arg == "--print-q-exit") {
            print_Q_exit = true;
        } else if (!arg.compare(0, 8, "--sleep=")) {
            try {
                sleep = std::stoll(arg.substr(8));
            } catch (const std::invalid_argument &) {
                std::cerr << "Ignored invalid option: " + arg << std::endl;
            }
        } else if (!arg.compare(0, 8, "--quiet=")) {
            try {
                quiet = std::stoll(arg.substr(8));
            } catch (const std::invalid_argument &) {
                std::cerr << "Ignored invalid option: " + arg << std::endl;
            }
        } else if (arg == "--random-device") {
            random_device = true;
#ifdef SokobanQLearning_USE_EMOJI_
        } else if (arg == "--emoji") {
            emoji = true;
#endif
#ifdef SokobanQLearning_CLI_USE_WINAPI_
        } else if (arg == "--ansi-escape") {
            ansi_escape = true;
#endif
        } else {
            std::cerr << "Ignored invalid argument: " + arg << std::endl;
        }
    }
    if (sleep < 0) sleep = 0;
    if (quiet < 0) quiet = 0;
    char c;
    std::string maze;
    while (std::cin >> std::noskipws >> c) maze += c;
    return RunAlgorithm<float, 64>(std::move(maze)) ? EXIT_SUCCESS : EXIT_FAILURE;
}
