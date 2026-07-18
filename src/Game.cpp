#include "Game.h"
#include "CollisionUtils.h"
#include <sstream>
#include <iomanip>

Game::Game()
    :window(sf::VideoMode((unsigned int)WIDTH, (unsigned int)HEIGHT), "OrbDash"),
     level(Level::testLevel()){
        window.setFramerateLimit(144);
        if (!font.loadFromFile("assets/font.ttf")){

    }
}

void Game::restart(){
    player.reset();
    cameraX = 0.f;
    dead = false;
    started = false;
}

void Game::handleInput(){
    sf::Event event;
    while (window.pollEvent(event)){
        if (event.type == sf::Event::Closed){
            window.close();
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space){
            if (dead){
                restart();
            }else {
                started = true;
                player.jump();
            }
        }
    }
}

void Game::update(float dt){
    if (dead || !started) return;

    cameraX += SCROLL_SPEED * dt;
    player.update(dt);

    float playerWorldX = 150.f + cameraX;
    for (const auto& obs : level.getObstacles()){
        bool hit;
        if (obs.type == ObstacleType::Block){
            float top = Player::getGroundY() - obs.height;
            hit = CollisionUtils::circleRectCollides(playerWorldX, player.getY(), Player::RADIUS,
                obs.worldX, top, obs.width, obs.height);
        }else {
            hit = CollisionUtils::circleTriangleCollides(playerWorldX, player.getY(), Player::RADIUS, 
                        obs.worldX, Player::getGroundY(), obs.width, obs.height);
        }
        if (hit){
            dead = true;
            break;
        }
    }

    if (cameraX >= level.getLength()){
        dead = true;
    }
}

void Game::render(){
    window.clear(sf::Color(0x1b, 0x1b, 0x2b));

    sf::RectangleShape ground(sf::Vector2f(WIDTH, HEIGHT - Player::getGroundY()));
    ground.setPosition(0.f, Player::getGroundY());
    ground.setFillColor(sf::Color(0x2c, 0x2c, 0x44));
    window.draw(ground);

    for (const auto& obs : level.getObstacles()){
        float screenX = obs.worldX - cameraX;
        if (screenX < -100.f || screenX > WIDTH + 100.f) continue;

        if (obs.type == ObstacleType::Block){
            sf::RectangleShape rect(sf::Vector2f(obs.width, obs.height));
            rect.setPosition(screenX, Player::getGroundY() - obs.height);
            rect.setFillColor(sf::Color(0xe8, 0xa3, 0x3d));
            window.draw(rect);
        }else {
            sf::ConvexShape tri(3);
            float baseY = Player::getGroundY();
            tri.setPoint(0, sf::Vector2f(screenX, baseY));
            tri.setPoint(1, sf::Vector2f(screenX + obs.width, baseY));
            tri.setPoint(2, sf::Vector2f(screenX + obs.width / 2.f, baseY - obs.height));
            tri.setFillColor(sf::Color(0xe8, 0xa3, 0x3d));
            window.draw(tri);
        }
    }

    player.render(window, 150.f);

    if (font.getInfo().family != ""){
        float progress = std::min(100.f, (cameraX / level.getLength()) * 100.f);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(0) << progress << "%";

        sf::Text progressText(ss.str(), font, 20);
        progressText.setFillColor(sf::Color::White);
        progressText.setPosition(WIDTH - 70.f, 20.f);
        window.draw(progressText);

        if (!started && !dead){
            sf::Text startText("Click or press SPACE to start", font, 24);
            startText.setFillColor(sf::Color::White);
            startText.setPosition(WIDTH / 2.f - 200.f, HEIGHT / 2.f - 60.f);
            window.draw(startText);
        }

        if (dead){
            sf::Text deadText("You died - click to retry", font, 32);
            deadText.setFillColor(sf::Color(0xff, 0x5e, 0x5e));
            deadText.setPosition(WIDTH / 2.f - 190.f, HEIGHT / 2.f - 20.f);
            window.draw(deadText);
        }
    }
    window.display();
}

void Game::run(){
    sf::Clock clock;
    while (window.isOpen()){
        float dt = clock.restart().asSeconds();
        if (dt > 0.05f) dt = 0.05f;

        handleInput();
        update(dt);
        render();
    }
}