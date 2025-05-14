#include <ncurses.h>
#include "kaka.h"

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    // ��������� ��������� ������
    if (has_colors()) {
        start_color();
    }
    
    Game game;
    game.run();
    
    endwin();
    return 0;
}