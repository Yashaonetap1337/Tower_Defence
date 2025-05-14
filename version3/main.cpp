#include <ncurses.h>
#include "kaka.h"

int main() {
    initscr();  // ������������� ncurses
    cbreak();   // ����� ��� �����������
    noecho();   // �� ���������� ���� ������������
    keypad(stdscr, TRUE);  // �������� ����. ������� (�������)

    Game game;
    game.run();

    endwin();  // ���������� ������ ncurses
    return 0;
}