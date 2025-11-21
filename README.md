# 2d engine for basic gravity simulation

## Implemented
- Gravity simulation between multiple planets
- Collision handling
- POSIX API for sockets and threading
- UDP client for planetary positions synchronization
- Planetary system configuration via JSON files
## Required
- CMake (4.1+ version)
- Ninja
## Build and run
```shell
git clone https://github.com/im-yasei/2d-engine.git
cd 2d-engine

#Run script
./build-release.sh

#Start simulation
./build-release/client/2d-engine
```

## Planet adding
To add a planet, you need to add a record about it to the assets/planets.json file
```json title:assets/planets.json
{
  "Planets": [
    {
      "x": 960,
      "y": 540,
      "radius": 50,
      "mass": 50000,
      "velocity": [0, 0],
      "color": [28, 100, 255]
    },
    {
      "x": 860,
      "y": 100,
      "radius": 20,
      "mass": 200,
      "velocity": [100, 10],
      "color": [200, 200, 200]
    }

  ]
}
```
### Json fields
- radius - planet radius
- mass - planet mass
- x, y - initial coordinates
- velocity - initial velocity vector (x, y)
- color - RGB color of the planet (optional)
## About physics
### Gravity implementation
Gravity is simulated using:
- Newton's law of universal gravitation $\large F = G\cdot \frac{M_1\cdot M_2}{R^2}$
- Newton's second law of motion $\large \overrightarrow a = \frac{\overrightarrow F}{m}$
- Using Euler's method to solve $\large \frac{d\overrightarrow V}{dt} = \overrightarrow a,$ $\large \frac{dx}{dt} = V_x,$ $\large \frac{dy}{dt} = V_y$ we get:
	 $\huge V_x = V_{0x} + a_x\cdot dt;\ \large V_y = V_{0y} + a_y\cdot dt$
	 $\huge x = x_0 + V_x\cdot dt;\ \large y = y_0 + V_y\cdot dt$

```cpp title:main.cpp
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
```
### Collision implementation
Collision is simulated using:
- Overlap calculation when planets intersect
- The scalar projections of the old velocities along the direction of collision:
	$\large V_{1n} = \overrightarrow{V_1} \cdot \overrightarrow{n}; \  \large V_{2n} = \overrightarrow{V_2} \cdot \overrightarrow{n},$
	where $\large \overrightarrow{n}$ is the unit vector perpendicular to the collision plane (collisionVector variable)
- The scalar projections of the new velocities along the direction of collision:
	$\huge U_{1n} = \frac{(m_1\  -\  em_2)\cdot V_{1n}\  +\  (1\  +\  e)\cdot m_2V_{2n}}{m_1\  +\  m_2}$;
	$\huge U_{2n} = \frac{(m_2\  -\  em_1)\cdot V_{2n}\  +\  (1\  +\  e)\cdot m_1V_{1n}}{m_1\  +\  m_2},$
	where $\large e$ is restitution coefficient
- Then we update the velocity vectors:
	Set $\large \overrightarrow V_1$ velocity to $\large \overrightarrow{V_1} + \overrightarrow{n} \cdot (U_{1n} - V_{1n})$; 
	Set $\large \overrightarrow V_2$ velocity to $\large \overrightarrow{V_2} + \overrightarrow{n} \cdot (U_{2n} - V_{2n})$

```cpp title:main.cpp
void applyCollision(std::vector<Planet> &planets) {
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

          float restitution = 0.9f;
          float u1n =
              ((m1 - restitution * m2) * v1n + (1 + restitution) * m2 * v2n) / (m1 + m2);
          float u2n =
              ((m2 - restitution * m1) * v2n + (1 + restitution) * m1 * v1n) / (m1 + m2);

          planets[i].setVelocity(v1 + collisionVector * (u1n - v1n));
          planets[j].setVelocity(v2 + collisionVector * (u2n - v2n));
        }
      }
    }
  }
}
```
