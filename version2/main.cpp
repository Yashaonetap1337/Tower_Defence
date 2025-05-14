#include <ncurses.h>
#include "kaka.h"

int main() {
    initscr();  
    cbreak();   
    noecho();   
    keypad(stdscr, TRUE); 

    Game game;
    game.run();

    endwin();  
    return 0;
}