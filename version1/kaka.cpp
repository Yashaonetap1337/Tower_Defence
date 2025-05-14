#include "kaka.h"
#include <cstdlib>

// Map implementation
Map::Map(int w, int h) : width(w), height(h) {
    grid.resize(height, std::vector<char>(width, ' '));
    generatePath();
}

void Map::generatePath() {
    for (int x = 0; x < width; x++) {
        path.push_back({x, height / 2});
        grid[height / 2][x] = '#';
    }
}

void Map::draw(WINDOW* win) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            mvwaddch(win, y, x, grid[y][x]);
        }
    }
}

bool Map::canPlaceTower(int x, int y) {
    return grid[y][x] == ' ';
}

void Map::placeTower(int x, int y) {
    if (canPlaceTower(x, y)) {
        grid[y][x] = 'T';
    }
}

// Tower implementations
Tower::Tower(int x, int y, int dmg, int rng, int c) : 
    x(x), y(y), damage(dmg), range(rng), cost(c) {}

void Tower::attack(Enemy& enemy) {
    if (inRange(enemy)) {
        enemy.takeDamage(damage);
    }
}

bool Tower::inRange(const Enemy& enemy) {
    int dx = enemy.getX() - x;
    int dy = enemy.getY() - y;
    return (dx * dx + dy * dy) <= (range * range);
}

int Tower::getCost() const { return cost; }
int Tower::getX() const { return x; }
int Tower::getY() const { return y; }

BasicTower::BasicTower(int x, int y) : Tower(x, y, 10, 3, 30) {}

SplashTower::SplashTower(int x, int y) : Tower(x, y, 15, 3, 50) {}

void SplashTower::attack(Enemy& enemy) {
    if (inRange(enemy)) {
        enemy.takeDamage(damage);
    }
}

// Enemy implementations
Enemy::Enemy(int startX, int startY, int hp, int spd, int rwd) :
    x(startX), y(startY), health(hp), speed(spd), reward(rwd) {}

void Enemy::move(const Map& map) {
    x += speed;
}

void Enemy::takeDamage(int dmg) {
    health -= dmg;
}

bool Enemy::isAlive() const { return health > 0; }
int Enemy::getX() const { return x; }
int Enemy::getY() const { return y; }
int Enemy::getReward() const { return reward; }

TankEnemy::TankEnemy(int startX, int startY) : 
    Enemy(startX, startY, 100, 1, 30) {}

// Player implementation
Player::Player() : money(100), health(100) {}

bool Player::isAlive() const { return health > 0; }
bool Player::canAfford(int amount) const { return money >= amount; }
void Player::spendMoney(int amount) { money -= amount; }
void Player::takeDamage(int damage) { health -= damage; }
void Player::addMoney(int amount) { money += amount; }

// WaveManager implementation
void WaveManager::spawnWave(std::vector<Enemy*>& enemies, int waveNumber) {
    int numEnemies = 5 + waveNumber * 2;
    for (int i = 0; i < numEnemies; i++) {
        if (rand() % 2 == 0) {
            enemies.push_back(new Enemy(0, 10, 30, 1, 10));
        } else {
            enemies.push_back(new TankEnemy(0, 10));
        }
    }
}

// Game implementation
Game::Game() : map(40, 20), currentWave(0), cursorX(1), cursorY(1) {}

bool Game::enemyReachedBase(const Enemy& enemy) const {
    return enemy.getX() >= map.getWidth() - 1;
}

void Game::run() {
    while (player.isAlive()) {
        handleInput();
        update();
        render();
        napms(100); // Задержка для плавности
    }
}

void Game::handleInput() {
    int ch = getch();
    switch (ch) {
        case KEY_UP:
            if (cursorY > 1) cursorY--;
            break;
        case KEY_DOWN:
            if (cursorY < map.getHeight() - 1) cursorY++;
            break;
        case KEY_LEFT:
            if (cursorX > 1) cursorX--;
            break;
        case KEY_RIGHT:
            if (cursorX < map.getWidth() - 1) cursorX++;
            break;
        case 't':
            if (map.canPlaceTower(cursorX, cursorY) && player.canAfford(30)) {
                towers.push_back(new BasicTower(cursorX, cursorY));
                map.placeTower(cursorX, cursorY);
                player.spendMoney(30);
            }
            break;
    }
}
void Game::update() {
    // Спавн новой волны, если нет врагов
    if (enemies.empty()) {
        currentWave++;
        waveManager.spawnWave(enemies, currentWave);
    }

    for (auto& enemy : enemies) {
        enemy->move(map);
        if (enemyReachedBase(*enemy)) {
            player.takeDamage(10);
        }
    }

    // Атака башен
    for (auto& tower : towers) {
        for (auto& enemy : enemies) {
            if (enemy->isAlive()) {
                tower->attack(*enemy);
            }
        }
    }

    // Удаление мёртвых врагов и награда
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [this](Enemy* e) {
            if (!e->isAlive()) {
                player.addMoney(e->getReward());
                delete e;
                return true;
            }
            return false;
        }), enemies.end());
}

void Game::render() {
    clear();
    map.draw(stdscr);
    
    // Рисуем башни
    for (auto& tower : towers) {
        mvprintw(tower->getY(), tower->getX(), "T");
    }
    
    // Рисуем врагов
    for (auto& enemy : enemies) {
        mvprintw(enemy->getY(), enemy->getX(), "E");
    }
    
    // Статус
    mvprintw(0, 0, "Wave: %d Money: %d Health: %d", currentWave, player.getMoney(), player.getHealth());
    
    mvprintw(cursorY, cursorX, "+");
    
    refresh();
}