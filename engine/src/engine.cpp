#include "engine.hpp"
#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Graphics/Color.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include <cmath>

Planet::Planet(float radius, float mass) {
  shape.setRadius(radius);
  shape.setOrigin(radius, radius);
  shape.setFillColor(sf::Color::Cyan);

  if (mass <= 0.0f) {
    this->mass = radius * radius;
  } else {
    this->mass = mass;
  }

  velocity = sf::Vector2f(0, 0);
  acceleration = sf::Vector2f(0, 0);
}

void Planet::setColor(int red, int green, int blue) {
  shape.setFillColor(sf::Color(red, green, blue));
}

void Planet::setRadius(float radius) {
  shape.setRadius(radius);
  shape.setOrigin(radius, radius);
}
float Planet::getRadius() const { return shape.getRadius(); }

void Planet::setPosition(sf::Vector2f planetPosition) {
  shape.setPosition(planetPosition);
}
sf::Vector2f Planet::getPosition() const { return shape.getPosition(); }

void Planet::setAcceleration(sf::Vector2f newAcceleration) {
  this->acceleration = newAcceleration;
}
sf::Vector2f Planet::getAcceleration() const { return this->acceleration; }

void Planet::setVelocity(sf::Vector2f newVelocity) {
  this->velocity = newVelocity;
}
sf::Vector2f Planet::getVelocity() const { return this->velocity; }

void Planet::setMass(float newMass) { this->mass = newMass; }
float Planet::getMass() const { return this->mass; }

sf::CircleShape Planet::getShape() const { return shape; }

bool Planet::isColliding(const Planet &other) const {
  sf::Vector2f delta = this->getPosition() - other.getPosition();
  float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
  return distance < (this->getRadius() + other.getRadius());
}

void Planet::draw(sf::RenderWindow &window) { window.draw(shape); }

void Planet::move(sf::Vector2f changePosition) { shape.move(changePosition); }
