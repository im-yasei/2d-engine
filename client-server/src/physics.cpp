#include "physics.hpp"
#include "engine.hpp"
#include <SFML/System/Vector2.hpp>
#include <X11/X.h>
#include <arpa/inet.h>
#include <cmath>
#include <fcntl.h>
#include <imgui-SFML.h>
#include <imgui.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

void applyGravity(std::vector<Planet> &planets, float G, float timeStep) {
  for (int i = 0; i < planets.size(); i++) {
    sf::Vector2f totalForce(0, 0);

    for (int j = 0; j < planets.size(); j++) {
      if (i == j)
        continue;

      sf::Vector2f direction =
          planets[j].getPosition() - planets[i].getPosition();

      float distance =
          std::sqrt(direction.x * direction.x + direction.y * direction.y);

      if (distance < 1.0f)
        continue;

      direction /= distance;

      float forceMagnitude = G * planets[i].getMass() * planets[j].getMass() /
                             (distance * distance);

      totalForce += direction * forceMagnitude;
    }

    sf::Vector2f newAcceleration = totalForce / planets[i].getMass();
    planets[i].setAcceleration(newAcceleration);

    sf::Vector2f newVelocity =
        planets[i].getVelocity() + newAcceleration * timeStep;
    planets[i].setVelocity(newVelocity);

    sf::Vector2f newPosition =
        planets[i].getPosition() + newVelocity * timeStep;
    planets[i].setPosition(newPosition);
  }
}

void applyCollision(std::vector<Planet> &planets) {
  const float FRICTION_COEFFICIENT = 0.08f;
  for (int i = 0; i < planets.size(); i++) {
    for (int j = i + 1; j < planets.size(); j++) {

      if (planets[i].isColliding(planets[j])) {

        sf::Vector2f collisionVector =
            planets[j].getPosition() - planets[i].getPosition();
        float distance = std::sqrt(collisionVector.x * collisionVector.x +
                                   collisionVector.y * collisionVector.y);

        if (distance > 0) {
          collisionVector /= distance;

          float m1 = planets[i].getMass(), m2 = planets[j].getMass();
          float totalMass = m1 + m2;
          float ratio1 = m2 / totalMass;
          float ratio2 = m1 / totalMass;

          float overlap =
              (planets[i].getRadius() + planets[j].getRadius()) - distance;
          planets[i].setPosition(planets[i].getPosition() -
                                 collisionVector * overlap * ratio1);
          planets[j].setPosition(planets[j].getPosition() +
                                 collisionVector * overlap * ratio2);

          sf::Vector2f v1 = planets[i].getVelocity();
          sf::Vector2f v2 = planets[j].getVelocity();

          float v1n = v1.x * collisionVector.x + v1.y * collisionVector.y;
          float v2n = v2.x * collisionVector.x + v2.y * collisionVector.y;

          float restitution = 0.0f;
          float u1n =
              ((m1 - restitution * m2) * v1n + (1 + restitution) * m2 * v2n) /
              (m1 + m2);
          float u2n =
              ((m2 - restitution * m1) * v2n + (1 + restitution) * m1 * v1n) /
              (m1 + m2);

          planets[i].setVelocity(v1 + collisionVector * (u1n - v1n));
          planets[j].setVelocity(v2 + collisionVector * (u2n - v2n));

          // friction
          // sf::Vector2f tangent(-collisionVector.y, collisionVector.x);
          //
          // float v1t = v1.x * tangent.x + v1.y * tangent.y;
          // float v2t = v2.x * tangent.x + v2.y * tangent.y;
          //
          // float relativeTangentialSpeed = std::abs(v1t - v2t);
          //
          // if (relativeTangentialSpeed > 0.1f) {
          //   v1t *= (1.0f - FRICTION_COEFFICIENT);
          //   v2t *= (1.0f - FRICTION_COEFFICIENT);
          //
          //   float new_v1n = planets[i].getVelocity().x * collisionVector.x +
          //                   planets[i].getVelocity().y * collisionVector.y;
          //   float new_v2n = planets[j].getVelocity().x * collisionVector.x +
          //                   planets[j].getVelocity().y * collisionVector.y;
          //
          //   planets[i].setVelocity(collisionVector * new_v1n + tangent *
          //   v1t); planets[j].setVelocity(collisionVector * new_v2n + tangent
          //   * v2t);
          // }
        }
      }
    }
  }
}
