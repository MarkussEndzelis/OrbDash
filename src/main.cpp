#include <SFML/Graphics.hpp>

int main(){
    sf::RenderWindow window(sf::VideoMode(400, 300), "SFML Test");
    sf::CircleShape circle(50);
    circle.setFillColor(sf::Color::Cyan);
    circle.setPosition(175, 100);

    while (window.isOpen()){
        sf::Event event;
        while (window.pollEvent(event)){
            if (event.type == sf::Event::Closed){
                window.close();
            }
        }
        window.clear(sf::Color::Black);
        window.draw(circle);
        window.display();
    }
    return 0;
}