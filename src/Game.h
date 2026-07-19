#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "Player.h"
#include "Level.h"

enum class GameState { MainMenu, LevelSelect, Playing};

struct LevelInfo {
    std::string name;
    std::string path;
};

class Game {
public:
    static constexpr float WIDTH = 900.f;
    static constexpr float HEIGHT = 500.f;

    Game();
    void run();

private:
    sf::RenderWindow window;
    sf::Font font;
    bool fontLoaded = false;
    Player player;
    Level level;

    GameState state = GameState::MainMenu;
    std::vector<LevelInfo> levels;

    float cameraX = 0.f;
    bool dead = false;
    bool won = false;
    bool started = false;

    static constexpr float SCROLL_SPEED = 360.f;
    static constexpr float BOOST_MULTIPLIER = 1.6f;
    bool boosting = false;

    void handleInput();
    void update(float dt);
    void render();
    void restart();

    void renderMainMenu();
    void renderLevelSelect();
    void renderPlaying();

    void handleMainMenuClick(sf::Vector2f mousePos);
    void handleLevelSelectClick(sf::Vector2f mousePos);

    void startLevel(int index, const std::string& path);

    sf::FloatRect playButtonBounds() const;
    std::vector<sf::FloatRect> levelButtonBounds() const;

    float elapsedTime = 0.f;
    std::vector<float> bestTimes;
    int currentLevelIndex = -1;

    void loadBestTimes();
    void saveBestTimes();
    std::string formatTime(float seconds) const;
};