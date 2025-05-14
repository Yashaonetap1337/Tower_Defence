
#ifndef KAKA_H
#define KAKA_H

#include <ncurses.h>
#include <vector>
#include <algorithm>
#include <chrono>

// Forward declarations
class Enemy;
class Player;
class WaveManager;

class Map {
private:
    int width, height;
    std::vector<std::vector<char>> grid;
    std::vector<std::pair<int, int>> path;

public:
    Map(int w, int h);
    void generatePath();
    void draw(WINDOW* win);
    bool canPlaceTower(int x, int y);
    void placeTower(int x, int y);
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    const std::vector<std::pair<int, int>>& getPath() const { return path; }
    void removeTower(int x, int y) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            grid[y][x] = ' '; 
        }
    }
};

class Tower {
protected:
    int x, y;
    int damage;
    int range;
    int cost;

public:
    Tower(int x, int y, int dmg, int rng, int c);
    virtual void attack(Enemy& enemy);
    bool inRange(const Enemy& enemy);
    int getCost() const;
    int getX() const;
    int getY() const;
};

class BasicTower : public Tower {
public:
    BasicTower(int x, int y);
};

class SplashTower : public Tower {
public:
    SplashTower(int x, int y);
    void attack(Enemy& enemy) override;
};

class Enemy {
protected:
    int x, y;
    int health;
    int speed;
    int reward;

public:
    Enemy(int startX, int startY, int hp, int spd, int rwd);
    void move(const Map& map);
    void takeDamage(int dmg);
    bool isAlive() const;
    int getX() const;
    int getY() const;
    int getReward() const;
    size_t currentPathIndex = 0;
    float progress = 0.0f;
};

class TankEnemy : public Enemy {
public:
    TankEnemy(int startX, int startY);
};

class Player {
private:
    int money;
    int health;

public:
    Player();
    bool isAlive() const;
    bool canAfford(int amount) const;
    void spendMoney(int amount);
    void takeDamage(int damage);
    void addMoney(int amount);
    int getHealth() const { return health; }
    int getMoney() const { return money; }
};

class WaveManager {
public:
    void spawnWave(std::vector<Enemy*>& enemies, int waveNumber);
};

class Game {
private:
    Map map;
    Player player;
    WaveManager waveManager;
    std::vector<Tower*> towers;
    std::vector<Enemy*> enemies;
    int currentWave;
    bool paused;
    std::chrono::steady_clock::time_point lastEnemyMoveTime;
    clock_t lastWaveSpawnTime;
    int gameSpeed;

    bool enemyReachedBase(const Enemy& enemy) const;
    Tower* getTowerAt(int x, int y) const {
        for (auto tower : towers) {
            if (tower->getX() == x && tower->getY() == y) {
                return tower;
            }
        }
        return nullptr;
    }
    void flash() {
        attron(A_REVERSE);
        mvaddch(cursorY, cursorX, ' ');
        refresh();
        napms(100);
        attroff(A_REVERSE);
    }

public:

    Game();
    void run();
    void handleInput();
    void togglePause() { paused = !paused; }
    void moveEnemies();
    void update();
    void render();
    int cursorX;
    int cursorY;
    ~Game();

};

#endif // KAKA_H