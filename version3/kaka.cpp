#include "kaka.h"
#include <cstdlib>
#include <ctime>
#include <climits>

// Map implementation
Map::Map(int w, int h) : width(w), height(h) {
    grid.resize(height, std::vector<char>(width, ' '));
    generatePath();
}

void Map::generatePath() {
    path.clear();
    
    #if defined(LEVEL_1)
    // Уровень 1: Прямой горизонтальный путь
    for (int x = 0; x < width; x++) {
        path.push_back({x, height / 2});
        grid[height / 2][x] = '#';
    }
    
    #elif defined(LEVEL_2)
    // Уровень 2: Зигзаг
    bool goingDown = true;
    int y = height / 3;
    
    for (int x = 0; x < width; x++) {
        path.push_back({x, y});
        grid[y][x] = '#';
        
        if (x % 5 == 0) {
            y += goingDown ? 1 : -1;
            if (y <= 1 || y >= height - 2) goingDown = !goingDown;
        }
    }
    
    #elif defined(LEVEL_3)
    // Уровень 3: Спираль
    int x = 0, y = height / 2;
    int dx = 1, dy = 0;
    int steps = 1;
    int stepCount = 0;
    int dirChanges = 0;
    
    while (x >= 0 && x < width && y >= 0 && y < height) {
        path.push_back({x, y});
        grid[y][x] = '#';
        
        x += dx;
        y += dy;
        stepCount++;
        
        if (stepCount == steps) {
            stepCount = 0;
            // Поворот направо
            int temp = dx;
            dx = -dy;
            dy = temp;
            dirChanges++;
            
            if (dirChanges % 2 == 0) {
                steps++;
            }
        }
    }
    
    #else
    // Уровень по умолчанию: Прямой путь
    for (int x = 0; x < width; x++) {
        path.push_back({x, height / 2});
        grid[height / 2][x] = '#';
    }
    #endif
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
    const auto& path = map.getPath();
    if (path.empty()) return;
    
    // Находим ближайшую точку пути
    size_t nearest = 0;
    int min_dist = INT_MAX;
    
    for (size_t i = 0; i < path.size(); i++) {
        int dx = path[i].first - x;
        int dy = path[i].second - y;
        int dist = dx*dx + dy*dy;
        
        if (dist < min_dist) {
            min_dist = dist;
            nearest = i;
        }
    }
    
    // Двигаемся к следующей точке
    if (nearest + 1 < path.size()) {
        x = path[nearest + 1].first;
        y = path[nearest + 1].second;
    }
}void Enemy::takeDamage(int dmg) {
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
Game::Game() : map(40, 20), currentWave(0), cursorX(0), cursorY(map.getHeight()/2) {}

Game::~Game() {
    // Очищаем башни
    for (auto tower : towers) {
        delete tower;
    }
    
    // Очищаем врагов
    for (auto enemy : enemies) {
        delete enemy;
    }
}

bool Game::enemyReachedBase(const Enemy& enemy) const {
    const auto& path = map.getPath();
    return !path.empty() && enemy.getX() == path.back().first && enemy.getY() == path.back().second;
}
void Game::run() {
    nodelay(stdscr, TRUE);  // Неблокирующий ввод
    curs_set(0);            // Скрываем курсор
    
    clock_t lastUpdate = clock();
    const int gameSpeed = 200; // ms между обновлениями
    
    while (player.isAlive()) {
        // Обработка ввода
        handleInput();
        
        // Игровая логика
        clock_t now = clock();
        if ((now - lastUpdate) * 1000 / CLOCKS_PER_SEC >= gameSpeed) {
            update();
            lastUpdate = now;
        }
        
        // Отрисовка
        render();
    }
    
    nodelay(stdscr, FALSE);
    curs_set(1);
    mvprintw(10, 10, "Game Over! Final Wave: %d", currentWave);
    refresh();
    getch();
}

void Game::handleInput() {
    int ch = getch();
    switch (ch) {
        case KEY_UP:
            if (cursorY > 0) cursorY--;
            break;
        case KEY_DOWN:
            if (cursorY < map.getHeight() - 1) cursorY++;
            break;
        case KEY_LEFT:
            if (cursorX > 0) cursorX--;
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
        case 'q':
            player.takeDamage(100);  // Для тестирования конца игры
            break;
    }
}

void Game::update() {
    // Спавн новой волны
    if (enemies.empty()) {
        currentWave++;
        waveManager.spawnWave(enemies, currentWave);
    }

    // Движение врагов
    for (auto& enemy : enemies) {
        enemy->move(map);
        if (enemyReachedBase(*enemy)) {
            player.takeDamage(10);
        }
    }

    // Атака башен
    for (auto& tower : towers) {
        for (auto& enemy : enemies) {
            if (enemy->isAlive() && tower->inRange(*enemy)) {
                tower->attack(*enemy);
            }
        }
    }

    // Удаление мёртвых врагов
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
    
    // Отрисовка пустого поля
    for (int y = 0; y < map.getHeight(); y++) {
        for (int x = 0; x < map.getWidth(); x++) {
            mvaddch(y, x, '.');  // Пустое пространство
        }
    }
    
    // Отрисовка пути из Map::path
    for (const auto& point : map.getPath()) {
        mvaddch(point.second, point.first, '#');
    }
    
    // Отрисовка башен
    for (auto& tower : towers) {
        mvaddch(tower->getY(), tower->getX(), 'T');
    }
    
    // Отрисовка врагов
    for (auto& enemy : enemies) {
        if (enemy->isAlive()) {
            mvaddch(enemy->getY(), enemy->getX(), 'E');
        }
    }
    
    // Отрисовка курсора
    mvaddch(cursorY, cursorX, '+');
    
    // Статусная информация
    mvprintw(0, 0, "Wave: %d Money: %d Health: %d", 
             currentWave, player.getMoney(), player.getHealth());
    
    // Отображение типа уровня
    #if defined(LEVEL_1)
    mvprintw(1, 0, "Level: Straight path");
    #elif defined(LEVEL_2)
    mvprintw(1, 0, "Level: Zigzag");
    #elif defined(LEVEL_3)
    mvprintw(1, 0, "Level: Spiral");
    #else
    mvprintw(1, 0, "Level: Default");
    #endif
    
    refresh();
}