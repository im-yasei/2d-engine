#pragma once
#include <SFML/Graphics.hpp>

class Planet {
private:
  int radius;
  sf::CircleShape shape;

public:
  void setRadius(int newRadius);
  int getRadius() const;
  void init();
  void draw(sf::RenderWindow &window);
  void setPosition(float x, float y);
};
