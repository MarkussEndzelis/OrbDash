#pragma once
#include <SFML/Graphics.hpp>
#include "Player.h"
#include "Level.h"

class Game {
public:
    static constexpr float WIDTH = 900.f;
    static constexpr float HEIGHT = 500.f;

    Game();
    void run();

private:
    sf::RenderWindow window;
    sf::Font font;
    Player player;
    Level level;

    float cameraX = 0.f;
    bool dead = false;
    bool started = false;

    static constexpr float SCROLL_SPEED = 360.f;

    void handleInput();
    void update(float dt);
    void render();
    void restart();
};