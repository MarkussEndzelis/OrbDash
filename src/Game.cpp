#include "Game.h"
#include "CollisionUtils.h"
#include <sstream>
#include <iomanip>
#include <fstream>

Game::Game()
    : window(sf::VideoMode(sf::Vector2u((unsigned int)WIDTH, (unsigned int)HEIGHT)), "OrbDash"),
      level(Level::testLevel())
{
    window.setFramerateLimit(144);
    fontLoaded = font.openFromFile("assets/font.ttf");

    levels = {
        { "Level 1 - Warmup", "assets/levels/level1.txt"},
        { "Level 2 - Rush", "assets/levels/level2.txt"},
        { "Level 3 - Ascent", "assets/levels/level3.txt"},
        { "Level 4 - Marathon", "assets/levels/level4.txt"}
    };

    bestTimes.assign(levels.size(), -1.f);
    loadBestTimes();
}

void Game::startLevel(int index, const std::string& path){
    currentLevelIndex = index;
    level = Level::loadFromFile(path);
    player.reset();
    cameraX = 0.f;
    dead = false;
    won = false;
    started = false;
    elapsedTime = 0.f;
    boosting = false;
    state = GameState::Playing;
}

void Game::restart() {
    player.reset();
    cameraX = 0.f;
    dead = false;
    won = false;
    started = false;
}

sf::FloatRect Game::playButtonBounds() const {
    return sf::FloatRect(sf::Vector2f(WIDTH / 2.f - 100.f, HEIGHT/ 2.f - 30.f), sf::Vector2f(200.f, 60.f));
}

std::vector<sf::FloatRect> Game::levelButtonBounds() const {
    std::vector<sf::FloatRect> bounds;
    int columns = 2;
    float boxWidth = 320.f;
    float boxHeight = 90.f;
    float spacingX = 30.f;
    float spacingY = 20.f;

    int rows = (int)((levels.size() + columns - 1) / columns);
    float totalWidth = columns * boxWidth + (columns - 1) * spacingX;
    float totalHeight = rows * boxHeight + (rows - 1) * spacingY;
    float startX = WIDTH / 2.f - totalWidth / 2.f;
    float startY = HEIGHT / 2.f - totalHeight / 2.f + 20.f;

    for (size_t i = 0; i < levels.size(); i++){
        int col = i % columns;
        int row = i / columns;
        float x = startX + col * (boxWidth + spacingX);
        float y = startY + row * (boxHeight + spacingY);
        bounds.push_back(sf::FloatRect(sf::Vector2f(x, y), sf::Vector2f(boxWidth, boxHeight)));
    }
    return bounds;
}

void Game::handleMainMenuClick(sf::Vector2f mousePos){
    if (playButtonBounds().contains(mousePos)){
        state = GameState::LevelSelect;
    }
}

void Game::handleLevelSelectClick(sf::Vector2f mousePos){
    auto bounds = levelButtonBounds();
    for (size_t i = 0; i < bounds.size(); i++){
        if (bounds[i].contains(mousePos)){
            startLevel((int)i, levels[i].path);
            return;
        }
    }
}

void Game::handleInput() {
    while (const std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if(state == GameState::Playing){
                if (keyPressed->code == sf::Keyboard::Key::Space) {
                    if (dead || won) {
                        state = GameState::LevelSelect;
                    }else if(!started){
                        started = true;
                    }else{
                        player.jump();
                    }
                }
                if (keyPressed->code == sf::Keyboard::Key::Escape){
                    state = GameState::LevelSelect;
                }
            }
        }
        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()){
            sf::Vector2f mousePos((float)mousePressed->position.x, (float)mousePressed->position.y);

            if (state == GameState::MainMenu){
                handleMainMenuClick(mousePos);
            }else if (state == GameState::LevelSelect){
                handleLevelSelectClick(mousePos);
            }else if (state == GameState::Playing){
                if (dead || won){
                    state = GameState::LevelSelect;
                }else if(!started){
                    started = true;
                }else {
                    player.jump();
                }
            }
        }
    }
}

void Game::update(float dt){
    if (state != GameState::Playing) return;
    if (dead || !started) return;

    boosting = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
               sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);

    float currentSpeed = SCROLL_SPEED * (boosting ? BOOST_MULTIPLIER : 1.f);
    cameraX += currentSpeed * dt;
    elapsedTime += dt;
    float playerWorldX = 150.f + cameraX;

    float floorY = Player::getGroundY();
    for (const auto& obs : level.getObstacles()){
        if (obs.type != ObstacleType::Platform) continue;

        float left = obs.worldX;
        float right = obs.worldX + obs.width;
        if (playerWorldX + Player::RADIUS < left || playerWorldX - Player::RADIUS > right) continue;

        float platformTop = Player::getGroundY() - obs.height;
        if (player.getY() - Player::RADIUS <= platformTop + 1.f){
            if (platformTop < floorY){
                floorY = platformTop;
            }
        }
    }

    player.update(dt, floorY);

    for (const auto& obs : level.getObstacles()){
        if (obs.type == ObstacleType::Platform) continue;

        bool hit;
        if (obs.type == ObstacleType::Block){
            float top = Player::getGroundY() - obs.height;
            hit = CollisionUtils::circleRectCollides(playerWorldX, player.getY(), Player::RADIUS,
                obs.worldX, top, obs.width, obs.height);
        }else {
            hit = CollisionUtils::circleTriangleCollides(playerWorldX, player.getY(), Player::RADIUS,
                obs.worldX, Player::getGroundY(), obs.width, obs.height);
        }
        if (hit) {
            dead = true;
            break;
        }
    }

    if (cameraX >= level.getLength()){
        won = true;
        started = false;

        if (currentLevelIndex >= 0){
            if (bestTimes[currentLevelIndex] < 0.f || elapsedTime < bestTimes[currentLevelIndex]){
                bestTimes[currentLevelIndex] = elapsedTime;
                saveBestTimes();
            }
            
        }
    }
}

void Game::renderMainMenu(){
    if (!fontLoaded) return;

    sf::Text title(font, "OrbDash", 48);
    title.setFillColor(sf::Color::White);
    title.setPosition(sf::Vector2f(WIDTH / 2.f - 100.f, HEIGHT / 2.f - 140.f));
    window.draw(title);

    sf::FloatRect btn = playButtonBounds();
    sf::RectangleShape btnShape(btn.size);
    btnShape.setPosition(btn.position);
    btnShape.setFillColor(sf::Color(0x5e, 0xc4, 0xff));
    window.draw(btnShape);

    sf::Text playText(font, "PLAY", 28);
    playText.setFillColor(sf::Color(0x1b, 0x1b, 0x2b));
    playText.setPosition(sf::Vector2f(btn.position.x + btn.size.x / 2.f - 35.f, btn.position.y + 12.f));
    window.draw(playText);
}

void Game::renderLevelSelect(){
    if (!fontLoaded) return;

    sf::Text title(font, "Select a level", 36);
    title.setFillColor(sf::Color::White);
    title.setPosition(sf::Vector2f(WIDTH / 2.f - 150.f, 80.f));
    window.draw(title);

    auto bounds = levelButtonBounds();
    for (size_t i = 0; i < bounds.size(); i++){
        sf::RectangleShape box(bounds[i].size);
        box.setPosition(bounds[i].position);
        box.setFillColor(sf::Color(0x2c, 0x2c, 0x44));
        box.setOutlineColor(sf::Color(0xe8, 0xa3, 0x3d));
        box.setOutlineThickness(2.f);
        window.draw(box);

        sf::Text label(font, levels[i].name, 18);
        label.setFillColor(sf::Color::White);
        label.setPosition(sf::Vector2f(bounds[i].position.x + 15.f, bounds[i].position.y + 30.f));
        window.draw(label);

        sf::Text timeText(font, "Best: " + formatTime(bestTimes[i]), 16);
        timeText.setFillColor(sf::Color(0xcc, 0xcc, 0xcc));
        timeText.setPosition(sf::Vector2f(bounds[i].position.x + 15.f, bounds[i].position.y + 58.f));
        window.draw(timeText);
    }
}

void Game::renderPlaying(){
    sf::RectangleShape ground(sf::Vector2f(WIDTH, HEIGHT - Player::getGroundY()));
    ground.setPosition(sf::Vector2f(0.f, Player::getGroundY()));
    ground.setFillColor(sf::Color(0x2c, 0x2c, 0x44));
    window.draw(ground);

    for (const auto& obs : level.getObstacles()){
        float screenX = obs.worldX - cameraX;
        if (screenX < -100.f || screenX > WIDTH + 100.f) continue;

        if (obs.type == ObstacleType::Block){
            sf::RectangleShape rect(sf::Vector2f(obs.width, obs.height));
            rect.setPosition(sf::Vector2f(screenX, Player::getGroundY() - obs.height));
            rect.setFillColor(sf::Color(0xe8, 0xa3, 0x3d));
            window.draw(rect);
        }else if (obs.type == ObstacleType::Spike){
            sf::ConvexShape tri(3);
            float baseY = Player::getGroundY();
            tri.setPoint(0, sf::Vector2f(screenX, baseY));
            tri.setPoint(1, sf::Vector2f(screenX + obs.width, baseY));
            tri.setPoint(2, sf::Vector2f(screenX + obs.width / 2.f, baseY - obs.height));
            tri.setFillColor(sf::Color(0xe8, 0xa3, 0x3d));
            window.draw(tri);
        }else if (obs.type == ObstacleType::Platform){
            float topY = Player::getGroundY() - obs.height;
            sf::RectangleShape plat(sf::Vector2f(obs.width, 14.f));
            plat.setPosition(sf::Vector2f(screenX, topY));
            plat.setFillColor(sf::Color(0x7c, 0xf2, 0x7c));
            window.draw(plat);
        }
    }

    player.render(window, 150.f);

    if (fontLoaded) {
        if (boosting && started && !dead && !won){
            sf::Text boostText(font, "BOOST", 20);
            boostText.setFillColor(sf::Color(0xff, 0xd0, 0x4d));
            boostText.setPosition(sf::Vector2f(20.f, 20.f));
            window.draw(boostText);
        }
        float progress = std::min(100.f, (cameraX / level.getLength()) * 100.f);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(0) << progress << "%";

        sf::Text progressText(font, ss.str(), 20);
        progressText.setFillColor(sf::Color::White);
        progressText.setPosition(sf::Vector2f(WIDTH - 70.f, 20.f));
        window.draw(progressText);

        if (!started && !dead && !won){
            sf::Text startText(font, "Click or press SPACE to start", 24);
            startText.setFillColor(sf::Color::White);
            startText.setPosition(sf::Vector2f(WIDTH / 2.f - 200.f, HEIGHT / 2.f - 60.f));
            window.draw(startText);
        }

        if (dead) {
            sf::Text deadText(font, "You died - click for level select", 32);
            deadText.setFillColor(sf::Color(0xff, 0x5e, 0x5e));
            deadText.setPosition(sf::Vector2f(WIDTH / 2.f - 230.f, HEIGHT / 2.f - 20.f));
            window.draw(deadText);
        }

        if (won) {
            sf::Text winText(font, "Level complete! - click for level select", 32);
            winText.setFillColor(sf::Color(0x7c, 0xf2, 0x7c));
            winText.setPosition(sf::Vector2f(WIDTH / 2.f - 280.f, HEIGHT / 2.f - 20.f));
            window.draw(winText);

            std::string bestStr = (currentLevelIndex >= 0) ? formatTime(bestTimes[currentLevelIndex]) : "--:--";
            sf::Text timeText(font, "Time: " + formatTime(elapsedTime) + "  Best: " + bestStr, 22);
            timeText.setFillColor(sf::Color::White);
            timeText.setPosition(sf::Vector2f(WIDTH / 2.f - 150.f, HEIGHT / 2.f + 30.f));
            window.draw(timeText);
        }
    }
}



void Game::render(){
    window.clear(sf::Color(0x1b, 0x1b, 0x2b));

    if (state == GameState::MainMenu){
        renderMainMenu();
    }else if (state == GameState::LevelSelect){
        renderLevelSelect();
    }else if (state == GameState::Playing){
        renderPlaying();
    }
    window.display();
}

void Game::run() {
    sf::Clock clock;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.05f) dt = 0.05f;

        handleInput();
        update(dt);
        render();
    }
}

void Game::loadBestTimes(){
    std::ifstream in("besttimes.txt");
    if(!in.is_open()) return;

    for (size_t i = 0; i < bestTimes.size() && in; i++){
        float t;
        if (in >> t){
            bestTimes[i] = t;
        }
    }
}

void Game::saveBestTimes(){
    std::ofstream out("besttimes.txt");
    for (float t : bestTimes){
        out << t << "\n";
    }
}

std::string Game::formatTime(float seconds) const {
    if (seconds < 0.f) return "--:--";
    int mins = (int)seconds / 60;
    float secs = seconds - mins * 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%05.2f", mins, secs);
    return std::string(buf);
}