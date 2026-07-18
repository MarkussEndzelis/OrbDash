#pragma once
#include <SFML/Graphics.hpp>

class Player {
public:
    static constexpr float RADIUS = 18.f;

    Player();

    void reset();
    void jump();
    void update(float dt);
    void render(sf::RenderWindow& window, float screenX);

    float getY() const {return y; }
    bool isOnGround() const {return onGround; }
    static float getGroundY();

private:
    float y;
    float velocityY;
    float rotation;
    bool onGround;

    static constexpr float GRAVITY = 2200.f;
    static constexpr float JUMP_VELOCITY = -820.f;
};