#include "GameState.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

namespace game {

namespace {

double rand01() {
    return static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
}

bool overlaps(const Rect& a, const Rect& b) {
    return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}

} // namespace

GameSession::GameSession() {
    reset();
}

void GameSession::reset() {
    bullets_.clear();
    enemyBullets_.clear();
    enemies_.clear();
    particles_.clear();

    score_ = 0;
    lives_ = 3;
    frame_ = 0;
    spawnTimer_ = 0;
    invincible_ = 120;
    cooldown_ = 0;

    player_.w = 32;
    player_.h = 32;
    player_.x = kFieldWidth / 2 - player_.w / 2;
    player_.y = kFieldHeight - 80;
}

void GameSession::handleStart() {
    reset();
    phase_ = Phase::Playing;
}

void GameSession::handleRetry() {
    reset();
    phase_ = Phase::Playing;
}

void GameSession::setInput(const InputState& input) {
    input_ = input;
}

void GameSession::spawnEnemy() {
    bool tough = rand01() < 0.15;
    Enemy e;
    e.w = 28;
    e.h = 28;
    e.x = rand01() * (kFieldWidth - e.w);
    e.y = -e.h;
    e.speed = tough ? (1.2 + rand01()) : (1.8 + rand01() * 1.5);
    e.hp = tough ? 3 : 1;
    e.tough = tough;
    e.shootTimer = 60 + rand01() * 60;
    e.vx = (rand01() - 0.5) * 1.2;
    enemies_.push_back(e);
}

void GameSession::spawnExplosion(double x, double y, const char* color) {
    for (int i = 0; i < 10; ++i) {
        Particle p;
        p.x = x;
        p.y = y;
        p.vx = (rand01() - 0.5) * 6;
        p.vy = (rand01() - 0.5) * 6;
        p.life = 20 + rand01() * 10;
        p.color = color;
        particles_.push_back(p);
    }
}

void GameSession::hitPlayer() {
    lives_--;
    invincible_ = 120;
    spawnExplosion(player_.x + player_.w / 2, player_.y + player_.h / 2, "#66d9ff");
    if (lives_ <= 0) {
        phase_ = Phase::GameOver;
    }
}

void GameSession::update() {
    if (phase_ != Phase::Playing) return;

    frame_++;

    if (input_.left) player_.x -= playerSpeed_;
    if (input_.right) player_.x += playerSpeed_;
    if (input_.up) player_.y -= playerSpeed_;
    if (input_.down) player_.y += playerSpeed_;
    player_.x = std::max(0.0, std::min(kFieldWidth - player_.w, player_.x));
    player_.y = std::max(0.0, std::min(kFieldHeight - player_.h, player_.y));

    if (cooldown_ > 0) cooldown_--;
    if (input_.fire && cooldown_ == 0) {
        Bullet b;
        b.w = 6;
        b.h = 14;
        b.x = player_.x + player_.w / 2 - b.w / 2;
        b.y = player_.y;
        b.speed = 9;
        bullets_.push_back(b);
        cooldown_ = fireRate_;
    }

    for (auto& b : bullets_) b.y -= b.speed;
    bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(),
                                   [](const Bullet& b) { return b.y + b.h <= 0; }),
                   bullets_.end());

    for (auto& b : enemyBullets_) b.y += b.speed;
    enemyBullets_.erase(std::remove_if(enemyBullets_.begin(), enemyBullets_.end(),
                                        [](const Bullet& b) { return b.y >= kFieldHeight; }),
                         enemyBullets_.end());

    spawnTimer_--;
    if (spawnTimer_ <= 0) {
        spawnEnemy();
        spawnTimer_ = std::max(20, 50 - static_cast<int>(score_ / 200));
    }

    for (auto& e : enemies_) {
        e.y += e.speed;
        e.x += e.vx;
        if (e.x < 0 || e.x + e.w > kFieldWidth) e.vx *= -1;
        e.shootTimer--;
        if (e.shootTimer <= 0 && e.y > 0 && e.y < kFieldHeight - 100) {
            Bullet b;
            b.w = 4;
            b.h = 10;
            b.x = e.x + e.w / 2 - b.w / 2;
            b.y = e.y + e.h;
            b.speed = 4;
            enemyBullets_.push_back(b);
            e.shootTimer = 90 + rand01() * 90;
        }
    }
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
                                   [](const Enemy& e) { return e.y >= kFieldHeight + 40; }),
                   enemies_.end());

    for (int i = static_cast<int>(bullets_.size()) - 1; i >= 0; --i) {
        Rect bRect{bullets_[i].x, bullets_[i].y, bullets_[i].w, bullets_[i].h};
        bool consumed = false;
        for (int j = static_cast<int>(enemies_.size()) - 1; j >= 0; --j) {
            Rect eRect{enemies_[j].x, enemies_[j].y, enemies_[j].w, enemies_[j].h};
            if (overlaps(bRect, eRect)) {
                enemies_[j].hp--;
                bullets_.erase(bullets_.begin() + i);
                consumed = true;
                if (enemies_[j].hp <= 0) {
                    spawnExplosion(enemies_[j].x + enemies_[j].w / 2, enemies_[j].y + enemies_[j].h / 2, "#ffcc66");
                    score_ += enemies_[j].tough ? 300 : 100;
                    enemies_.erase(enemies_.begin() + j);
                }
                break;
            }
        }
        if (consumed) continue;
    }

    if (invincible_ > 0) {
        invincible_--;
    } else {
        Rect pRect{player_.x, player_.y, player_.w, player_.h};
        for (int j = static_cast<int>(enemies_.size()) - 1; j >= 0; --j) {
            Rect eRect{enemies_[j].x, enemies_[j].y, enemies_[j].w, enemies_[j].h};
            if (overlaps(pRect, eRect)) {
                spawnExplosion(enemies_[j].x + enemies_[j].w / 2, enemies_[j].y + enemies_[j].h / 2, "#ff5555");
                enemies_.erase(enemies_.begin() + j);
                hitPlayer();
                break;
            }
        }
        for (int i = static_cast<int>(enemyBullets_.size()) - 1; i >= 0; --i) {
            Rect bRect{enemyBullets_[i].x, enemyBullets_[i].y, enemyBullets_[i].w, enemyBullets_[i].h};
            if (overlaps(pRect, bRect)) {
                enemyBullets_.erase(enemyBullets_.begin() + i);
                hitPlayer();
                break;
            }
        }
    }

    for (auto& p : particles_) {
        p.x += p.vx;
        p.y += p.vy;
        p.life--;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                                     [](const Particle& p) { return p.life <= 0; }),
                      particles_.end());
}

std::string GameSession::toJson() const {
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out.precision(1);

    out << "{";
    out << "\"phase\":\"" << (phase_ == Phase::Start ? "start" : phase_ == Phase::Playing ? "playing" : "gameover") << "\",";
    out << "\"score\":" << score_ << ",";
    out << "\"lives\":" << lives_ << ",";
    out << "\"invincible\":" << (invincible_ > 0 ? "true" : "false") << ",";

    out << "\"player\":{\"x\":" << player_.x << ",\"y\":" << player_.y
        << ",\"w\":" << player_.w << ",\"h\":" << player_.h << "},";

    out << "\"bullets\":[";
    for (size_t i = 0; i < bullets_.size(); ++i) {
        if (i > 0) out << ",";
        const auto& b = bullets_[i];
        out << "{\"x\":" << b.x << ",\"y\":" << b.y << ",\"w\":" << b.w << ",\"h\":" << b.h << "}";
    }
    out << "],";

    out << "\"enemyBullets\":[";
    for (size_t i = 0; i < enemyBullets_.size(); ++i) {
        if (i > 0) out << ",";
        const auto& b = enemyBullets_[i];
        out << "{\"x\":" << b.x << ",\"y\":" << b.y << ",\"w\":" << b.w << ",\"h\":" << b.h << "}";
    }
    out << "],";

    out << "\"enemies\":[";
    for (size_t i = 0; i < enemies_.size(); ++i) {
        if (i > 0) out << ",";
        const auto& e = enemies_[i];
        out << "{\"x\":" << e.x << ",\"y\":" << e.y << ",\"w\":" << e.w << ",\"h\":" << e.h
            << ",\"tough\":" << (e.tough ? "true" : "false") << "}";
    }
    out << "],";

    out << "\"particles\":[";
    for (size_t i = 0; i < particles_.size(); ++i) {
        if (i > 0) out << ",";
        const auto& p = particles_[i];
        out << "{\"x\":" << p.x << ",\"y\":" << p.y << ",\"life\":" << p.life
            << ",\"color\":\"" << p.color << "\"}";
    }
    out << "]";

    out << "}";
    return out.str();
}

} // namespace game
