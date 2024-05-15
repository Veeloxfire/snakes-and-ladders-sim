#include <cstdio>
#include <unordered_map>
#include <random>
#include <thread>
#include <cassert>
#include <cassert>
#include <Windows.h>

#define SPEED
//#define RENDER_BOARD

enum struct RenderLocation {
  Start = 0,
#ifdef RENDER_BOARD
  Middle = 1,
#endif
  End = 2
};

struct RenderInfo {
  RenderLocation render_loc;
  int info_counter;
  int render_every;

#ifdef RENDER_BOARD
  int board_counter;
#endif

  void render_info(int goes, int max_goes, int total_turns, int last_turns, int best_turns, int worst_turns) {
    const bool render_this = info_counter == 0
#ifdef RENDER_BOARD
      || board_counter == 0;
#endif
      ;
    info_counter += 1;
    info_counter %= render_every;


    if (!render_this) return;


    constexpr char clear_line[] = "\x1b[K";

    switch (render_loc) {
      case RenderLocation::Start: {
#ifdef RENDER_BOARD
          fputs("\x1b[28E", stdout);
#endif
          break;
        }
#ifdef RENDER_BOARD
      case RenderLocation::Middle: {
          break;
        }
#endif
      case RenderLocation::End: {
          fputs("\x1b[6F", stdout);
          break;
        }
    }


    fputs(clear_line, stdout);
    fprintf(stdout, "wins: %d/%d\n", goes, max_goes);
    fputs(clear_line, stdout);
    fprintf(stdout, "total turns: %d\n", total_turns);
    fputs(clear_line, stdout);
    fprintf(stdout, "average turns: %f\n", ((float)total_turns / (float)(goes)));
    fputs(clear_line, stdout);
    fprintf(stdout, "last took: %d turns\n", last_turns);
    fputs(clear_line, stdout);
    fprintf(stdout, "best took: %d turns\n", best_turns);
    fputs(clear_line, stdout);
    fprintf(stdout, "worst took: %d turns\n", worst_turns);

    render_loc = RenderLocation::End;
  }

#ifdef RENDER_BOARD
  void render_board(int curr_pos, const std::unordered_map<int, int>& snakes) {
    const bool render_this = board_counter == 0;
    board_counter += 1;
    board_counter %= render_every;

    if (!render_this) {
      return;
    }

    switch (render_loc) {
      case RenderLocation::Start: {
          break;
        }
      case RenderLocation::Middle: {
          fputs("\x1b[22F", stdout);
          break;
        }
      case RenderLocation::End: {
          fputs("\x1b[28F", stdout);
          break;
        }
    }

    constexpr char line[] = "+-+-+-+-+-+-+-+-+-+-+\n";
    fputs(line, stdout);

    for (int i = 0; i < 10; ++i) {
      char arr[] = "| | | | | | | | | | |\n";
      for (int j = 0; j < 10; ++j) {
        int j_idx = (i % 2 == 1 ? j : 9 - j);
        int idx = (90 - i * 10) + j_idx;
        if (idx == curr_pos) arr[j * 2 + 1] = 'p';
        else if (snakes.find(idx + 1) != snakes.end()) arr[j * 2 + 1] = 'x';
      }
      fputs(arr, stdout);
      fputs(line, stdout);
    }


    fputc('\n', stdout);

    render_loc = RenderLocation::Middle;

#ifndef SPEED
    //Lets not obilterate the cpu
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1ms);
#endif
  }

#endif
};

const std::unordered_map<int, int> only_snakes = {
  {14, 4}, {17, 7}, {31, 9}, {38, 20},
  {54, 34}, {59, 40}, {62, 19}, {64, 60},
  {67, 51}, {81, 63}, {84, 28}, {87, 24},
  {91, 71}, {93, 73}, {95, 75}, {99, 78},
};

const std::unordered_map<int, int> snakes_and_ladders = {
  {4, 14}, {17, 7}, {9, 31}, {20, 38},
  {54, 34}, {40, 59}, {62, 19}, {64, 60},
  {51, 67}, {63, 81}, {28, 84}, {87, 24},
  {71, 91}, {93, 73}, {95, 75}, {99, 78},
};

int main(int argc, const char** argv) {
  if (argc != 2) {
    fputs("Invalid argument count (expected 1 number)\n", stdout);
    return 1;
  }

  {
    const auto std_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD flags;
    GetConsoleMode(std_handle, &flags);
    SetConsoleMode(std_handle, flags | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  }

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dice_roll(1, 6);

  const std::unordered_map<int, int>& pipes = snakes_and_ladders;

  int best_turns = INT_MAX;
  int worst_turns = 0;

  int total_turns = 0;

  RenderInfo render = {};
  render.render_every = 100;
  render.info_counter = 0;
  render.render_loc = RenderLocation::Start;
#ifdef RENDER_BOARD
  render.board_counter = 0;
#endif

  int max_goes;
  if (sscanf_s(argv[1], "%d", &max_goes) != 1) {
    fputs("Invalid arguments\n", stdout);
    return 1;
  }

  int turns = 0;

  for (int _goes = 0; _goes < max_goes; ++_goes) {
    turns = 0;
    int curr_pos = 0;

#ifdef RENDER_BOARD
    render.render_board(curr_pos, pipes);
#endif

    int add;
    do {
      add = dice_roll(gen);
      if (curr_pos + add <= 99) {
        curr_pos += add;
      }

#ifdef RENDER_BOARD
      render.render_board(curr_pos, pipes);
#endif

      const auto maybe = pipes.find(curr_pos + 1);
      if (maybe != pipes.end()) {
        curr_pos = maybe->second - 1;
#ifdef RENDER_BOARD
        render.render_board(curr_pos, pipes);
#endif
      }

      turns += 1;
    } while (curr_pos != 99);

    if (turns < best_turns) best_turns = turns;
    if (turns > worst_turns) worst_turns = turns;
    total_turns += turns;

    render.render_info(_goes + 1, max_goes, total_turns, turns, best_turns, worst_turns);
    if (render.info_counter == 1) {
      // add a bit of randomness to the printing
      render.info_counter += add % (render.render_every - render.info_counter);
    }

#ifndef SPEED
    //Lets not obilterate the cpu
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1ms);
#endif
  }

  render.info_counter = 0;
  render.render_info(max_goes, max_goes, total_turns, turns, best_turns, worst_turns);
}

