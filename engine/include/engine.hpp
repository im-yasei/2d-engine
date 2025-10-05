#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>

class Planet {
private:
  int radius;
  sf::CircleShape shape;

public:
  void setRadius(float newRadius);
  int getRadius() const;
  void init();
  void draw(sf::RenderWindow &window);
  void setPosition(sf::Vector2f planetPosition);
  void move(sf::Vector2f changePosition);
};
