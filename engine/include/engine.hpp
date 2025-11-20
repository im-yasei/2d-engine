#pragma once
#include "SFML/Graphics/CircleShape.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>

class Planet {
private:
  sf::CircleShape shape;
  float mass;
  sf::Vector2f velocity;
  sf::Vector2f acceleration;

public:
  Planet(float radius = 10.0f, float mass = 0.0f);

  void setColor(int red, int glue, int blue);

  void setRadius(float newRadius);
  float getRadius() const;

  void setMass(float m);
  float getMass() const;

  void setVelocity(sf::Vector2f v);
  sf::Vector2f getVelocity() const;

  void setAcceleration(sf::Vector2f a);
  sf::Vector2f getAcceleration() const;

  void setPosition(sf::Vector2f planetPosition);
  sf::Vector2f getPosition() const;

  sf::CircleShape getShape() const;

  bool isColliding(const Planet &other) const;

  void draw(sf::RenderWindow &window);
  void move(sf::Vector2f changePosition);
};
