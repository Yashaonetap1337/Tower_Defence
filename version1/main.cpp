#include <ncurses.h>
#include "kaka.h"

int main() {
    initscr();  // Инициализация ncurses
    cbreak();   // Режим без буферизации
    noecho();   // Не отображать ввод пользователя
    keypad(stdscr, TRUE);  // Включить спец. клавиши (стрелки)

    Game game;
    game.run();

    endwin();  // Завершение работы ncurses
    return 0;
}