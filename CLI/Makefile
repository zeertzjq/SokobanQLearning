CXXFLAGS = -std=c++14
ifeq ($(DEBUG), y)
ifeq ($(CXX), clang++)
CXXFLAGS += -Weverything
endif
CXXFLAGS += -Wall -Wextra -pedantic -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-weak-vtables -Wno-conversion -Wno-sign-compare -Wno-float-equal -g -Og
else
CXXFLAGS += -O3 -D SokobanQLearning_USE_EMOJI_
endif
ifeq ($(OS), Windows_NT)
ifeq ($(CXX), clang++)
CXXFLAGS += -Wno-nonportable-system-include-path
endif
OUT = CLI.exe
else
OUT = CLI
endif

build:
	$(CXX) $(CXXFLAGS) CLI.cpp -o $(OUT)
