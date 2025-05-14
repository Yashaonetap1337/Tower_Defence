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
    const int roadWidth = 3; 
    
    #if defined(LEVEL_1)
    // Прямая дорога шириной 3 клетки
    for (int x = 0; x < width; x++) {
        for (int dy = -1; dy <= 1; dy++) {
            int y = height/2 + dy;
            if(y >= 0 && y < height) {
                path.push_back({x, y});
                grid[y][x] = '#';
            }
        }
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

void Tower::attack(Enemy& enemy, std::vector<Projectile>& projectiles) {
    if (inRange(enemy)) {
        projectiles.emplace_back(x, y, &enemy, damage);
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

BasicTower::BasicTower(int x, int y) : Tower(x, y, 10, 10, 30) {}

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
    if (path.empty() || currentPathIndex >= path.size() - 1) return;

    // Плавное движение между точками
    if (progress < 1.0f) {
        progress += 0.1f * speed; // Скорость движения
        int nextIndex = currentPathIndex + 1;
        x = static_cast<int>(path[currentPathIndex].first + 
                           (path[nextIndex].first - path[currentPathIndex].first) * progress);
        y = static_cast<int>(path[currentPathIndex].second + 
                           (path[nextIndex].second - path[currentPathIndex].second) * progress);
    } else {
        currentPathIndex++;
        progress = 0.0f;
    }
}
void Enemy::takeDamage(int dmg) {
    health -= dmg;
}

bool Enemy::isAlive() const { return health > 0; }
int Enemy::getX() const { return x; }
int Enemy::getY() const { return y; }
int Enemy::getReward() const { return reward; }

TankEnemy::TankEnemy(int startX, int startY) : 
    Enemy(startX, startY, 100000, 1, 30) {}

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
Game::Game() : map(150, 55), currentWave(0), cursorX(0), cursorY(map.getHeight()/2) {}

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
    nodelay(stdscr, TRUE);
    curs_set(0);
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);

    auto lastUpdate = std::chrono::steady_clock::now();
    auto lastEnemyMove = lastUpdate;
    constexpr std::chrono::milliseconds frameDelay(50); // 50ms для общего обновления
    constexpr std::chrono::milliseconds enemyMoveInterval(200); // 200ms для движения врагов

    while (player.isAlive()) {
        auto now = std::chrono::steady_clock::now();
        
        handleInput();

        // Обновление игровой логики
        if (now - lastUpdate >= frameDelay) {
            update();
            lastUpdate = now;
        }

        // Отдельное обновление для движения врагов
        if (now - lastEnemyMove >= enemyMoveInterval) {
            moveEnemies();
            lastEnemyMove = now;
        }

        render();
        napms(10); // Короткая задержка для отзывчивости управления
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
            
        case 't': {  // Постройка башни
            if (getTowerAt(cursorX, cursorY) != nullptr) break;
            
            if (map.canPlaceTower(cursorX, cursorY)) {
                if (player.canAfford(30)) {
                    towers.push_back(new BasicTower(cursorX, cursorY));
                    map.placeTower(cursorX, cursorY);
                    player.spendMoney(30);
                } else {
                    mvprintw(3, 0, "Not enough gold! Need: 30");
                    refresh();
                }
            }
            break;
        }
        
        case 's': {  // Продажа башни
            Tower* tower = getTowerAt(cursorX, cursorY);
            if (tower != nullptr) {
                player.addMoney(tower->getCost() / 2);
                map.removeTower(cursorX, cursorY);
                towers.erase(std::remove(towers.begin(), towers.end(), tower), towers.end());
                delete tower;
                flash();
            }
            break;
        }
        
        case 'q':
            player.takeDamage(100);
            break;
    }
}
void Game::moveEnemies() {
    for (auto& enemy : enemies) {
        enemy->move(map);
        if (enemyReachedBase(*enemy)) {
            player.takeDamage(10);
        }
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
                tower->attack(*enemy, projectiles);
            }
        }
    }
    
    updateProjectiles();
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

void Game::updateProjectiles() {
    auto it = projectiles.begin();
    while (it != projectiles.end()) {
        // Обновляем позицию
        it->progress += projectileSpeed;
        Enemy* target = it->target;
        
        if (target && target->isAlive()) {
            // Рассчитываем промежуточную позицию
            int dx = target->getX() - it->startX;
            int dy = target->getY() - it->startY;
            
            it->currentX = it->startX + static_cast<int>(dx * it->progress);
            it->currentY = it->startY + static_cast<int>(dy * it->progress);
            
            // Проверяем достижение цели
            if (it->progress >= 1.0f) {
                target->takeDamage(it->damage);
                it = projectiles.erase(it);
            } else {
                ++it;
            }
        } else {
            // Цель мертва - удаляем снаряд
            it = projectiles.erase(it);
        }
    }
}

void Game::render() {
    clear();
    
    // Отрисовка границ поля
    const int W = map.getWidth();
    const int H = map.getHeight();
    
    // Верхняя и нижняя границы
    for (int x = 0; x < W; x++) {
        mvaddch(0, x, (x == 0 || x == W-1) ? '+' : '-');
        mvaddch(H-1, x, (x == 0 || x == W-1) ? '+' : '-');
    }
    
    // Боковые границы
    for (int y = 1; y < H-1; y++) {
        mvaddch(y, 0, '|');
        mvaddch(y, W-1, '|');
    }

    // Отрисовка пути из Map::path
    for (const auto& point : map.getPath()) {
        if (point.first > 0 && point.first < W-1 && 
            point.second > 0 && point.second < H-1) {
            mvaddch(point.second, point.first, '#' | COLOR_PAIR(1));
        }
    }
    
    // Отрисовка башен
    for (auto& tower : towers) {
        int x = tower->getX();
        int y = tower->getY();
        if (x > 0 && x < W-1 && y > 0 && y < H-1) {
            mvaddch(y, x, 'T' | A_BOLD);
        }
    }
    
    // Отрисовка врагов
    for (auto& enemy : enemies) {
        if (enemy->isAlive()) {
            int x = enemy->getX();
            int y = enemy->getY();
            if (x > 0 && x < W-1 && y > 0 && y < H-1) {
                mvaddch(y, x, 'E' | COLOR_PAIR(2));
            }
        }
    }
        // Отрисовка снарядов
    for (const auto& projectile : projectiles) {
        if (projectile.currentX >= 0 && projectile.currentX < map.getWidth() &&
            projectile.currentY >= 0 && projectile.currentY < map.getHeight()) {
            mvaddch(projectile.currentY, projectile.currentX, '*');
        }
    }
    
    // Отрисовка курсора
    if (cursorX > 0 && cursorX < W-1 && 
        cursorY > 0 && cursorY < H-1) {
        mvaddch(cursorY, cursorX, '+' | A_BOLD);
    }
        // Статусная информация
    mvprintw(0, 0, "Wave: %d Money: %d Health: %d", 
             currentWave, player.getMoney(), player.getHealth());
    mvprintw(2, 0, "T: Build | S: Sell | Q: Quit");
    
    // Отображаем стоимость продажи если есть башня
    Tower* tower = getTowerAt(cursorX, cursorY);
    if (tower != nullptr) {
        mvprintw(3, 0, "Sell for: %d gold", tower->getCost() / 2);
    }
    
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