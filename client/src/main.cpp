#include "engine.hpp"

int main() {
  sf::RenderWindow window(sf::VideoMode(800, 600), "cyan planet");

  Planet planet;
  planet.setRadius(50);
  planet.init();
  planet.setPosition(100, 100);

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed)
        window.close();
    }

    window.clear();
    planet.draw(window);
    window.display();
  }
  return 0;
}
