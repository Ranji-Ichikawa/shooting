#pragma once
#include <string>
#include <vector>

namespace game {

constexpr double kFieldWidth = 480.0;
constexpr double kFieldHeight = 640.0;

struct Rect {
    double x = 0, y = 0, w = 0, h = 0;
};

struct Bullet {
    double x, y, w, h, speed;
};

struct Enemy {
    double x, y, w, h;
    double speed;
    double vx;
    int hp;
    double shootTimer;
    bool tough;
};

struct Particle {
    double x, y, vx, vy;
    double life;
    const char* color;
};

// Snapshot of which controls the player is currently holding down.
// Sent by the client on every key state change (see Protocol::parseInput).
struct InputState {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool fire = false;
};

enum class Phase { Start, Playing, GameOver };

// One player's full game state, owned server-side. The client never runs
// game logic itself -- it only renders whatever snapshot GameSession::toJson
// produces and forwards input/commands back.
class GameSession {
public:
    GameSession();

    void handleStart();
    void handleRetry();
    void setInput(const InputState& input);

    // Advances the simulation by one tick. No-op unless Phase::Playing.
    void update();

    std::string toJson() const;

private:
    void reset();
    void spawnEnemy();
    void spawnExplosion(double x, double y, const char* color);
    void hitPlayer();

    Phase phase_ = Phase::Start;

    Rect player_;
    double playerSpeed_ = 5.0;
    int cooldown_ = 0;
    int fireRate_ = 8;

    std::vector<Bullet> bullets_;
    std::vector<Bullet> enemyBullets_;
    std::vector<Enemy> enemies_;
    std::vector<Particle> particles_;

    int score_ = 0;
    int lives_ = 3;
    long frame_ = 0;
    int spawnTimer_ = 0;
    int invincible_ = 0;

    InputState input_;
};

} // namespace game
