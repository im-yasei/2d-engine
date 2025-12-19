#include "client-server.hpp"
#include "engine.hpp"
#include "physics.hpp"
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

std::atomic<bool> clientRunning{true};

uint32_t float_to_network(float value) {
  uint32_t result;
  memcpy(&result, &value, sizeof(float));
  return htonl(result);
}

float network_to_float(uint32_t value) {
  value = ntohl(value);
  float result;
  memcpy(&result, &value, sizeof(float));
  return result;
}

void server_send_broadcast(int sockfd, std::vector<Planet> *planets,
                           std::mutex *planets_mutex, int port,
                           const std::string &ip, float *G, float *timeStep) {
  struct sockaddr_in broadcast_addr;
  memset(&broadcast_addr, 0, sizeof(broadcast_addr));

  broadcast_addr.sin_family = AF_INET;
  broadcast_addr.sin_port = htons(port);
  broadcast_addr.sin_addr.s_addr = inet_addr(ip.c_str());

  const size_t MAX_UDP_PAYLOAD = 65507;

  while (clientRunning) {
    {
      std::lock_guard<std::mutex> lock(*planets_mutex);
      applyGravity(*planets, *G, *timeStep);
      applyCollision(*planets);
    }

    std::vector<char> buffer;

    {
      std::lock_guard<std::mutex> lock(*planets_mutex);

      int32_t num_planets = static_cast<int32_t>(planets->size());

      size_t required_size =
          sizeof(int32_t) + num_planets * 6 * sizeof(float) + num_planets * 3;

      if (required_size > MAX_UDP_PAYLOAD) {
        int32_t max_planets =
            (MAX_UDP_PAYLOAD - sizeof(int32_t)) / (6 * sizeof(float) + 3);
        num_planets = max_planets;
        required_size = sizeof(int32_t) + num_planets * (6 * sizeof(float) + 3);

        std::cerr << "Warning: too many planets (" << planets->size()
                  << "), truncating to " << num_planets
                  << " (packet size: " << required_size << " bytes)"
                  << std::endl;
      }

      buffer.resize(required_size);
      char *ptr = buffer.data();

      int32_t net_num_planets = htonl(num_planets);
      memcpy(ptr, &net_num_planets, sizeof(int32_t));
      ptr += sizeof(int32_t);

      for (int i = 0; i < num_planets; i++) {
        const auto &planet = planets->at(i);
        float x = planet.getPosition().x;
        float y = planet.getPosition().y;
        float r = planet.getRadius();
        float m = planet.getMass();
        float velocity_x = planet.getVelocity().x;
        float velocity_y = planet.getVelocity().y;

        std::uint8_t color_r = planet.getShape().getFillColor().r;
        std::uint8_t color_g = planet.getShape().getFillColor().g;
        std::uint8_t color_b = planet.getShape().getFillColor().b;

        uint32_t net_x = float_to_network(x);
        uint32_t net_y = float_to_network(y);
        uint32_t net_r = float_to_network(r);
        uint32_t net_m = float_to_network(m);
        uint32_t net_velocity_x = float_to_network(velocity_x);
        uint32_t net_velocity_y = float_to_network(velocity_y);

        memcpy(ptr, &net_x, sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        memcpy(ptr, &net_y, sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        memcpy(ptr, &net_r, sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        memcpy(ptr, &net_m, sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        memcpy(ptr, &net_velocity_x, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        memcpy(ptr, &net_velocity_y, sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        *ptr++ = color_r;
        *ptr++ = color_g;
        *ptr++ = color_b;
      }
    }

    int bytes_sent = sendto(sockfd, buffer.data(), buffer.size(), 0,
                            (const struct sockaddr *)&broadcast_addr,
                            sizeof(broadcast_addr));

    if (bytes_sent < 0) {
      perror("sendto failed in broadcast");
    } else if (bytes_sent != static_cast<int>(buffer.size())) {
      std::cerr << "Warning: sent " << bytes_sent << " bytes, expected "
                << buffer.size() << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void client_receive(int sockfd, std::vector<Planet> *planets,
                    std::mutex *planets_mutex,
                    std::vector<std::vector<sf::Vertex>> *trajectories) {
  struct sockaddr_in sender_addr;
  socklen_t sender_len = sizeof(sender_addr);
  char buffer[65507];

  while (clientRunning) {
    int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                  (struct sockaddr *)&sender_addr, &sender_len);

    if (bytes_received > 0) {
      if (bytes_received >= static_cast<int>(sizeof(int32_t))) {

        int32_t num_planets;
        memcpy(&num_planets, buffer, sizeof(int32_t));
        num_planets = ntohl(num_planets);

        size_t expected_size =
            sizeof(int32_t) + num_planets * 6 * sizeof(float) + num_planets * 3;
        if (bytes_received >= static_cast<int>(expected_size)) {

          const char *data_ptr = buffer + sizeof(int32_t);

          std::vector<Planet> new_planets;
          new_planets.reserve(num_planets);

          std::vector<std::vector<sf::Vertex>> new_trajectories;
          new_trajectories.reserve(num_planets);

          for (int i = 0; i < num_planets; i++) {
            uint32_t net_x, net_y, net_r, net_m, net_vx, net_vy;

            memcpy(&net_x, data_ptr, sizeof(uint32_t));
            data_ptr += sizeof(uint32_t);
            memcpy(&net_y, data_ptr, sizeof(uint32_t));
            data_ptr += sizeof(uint32_t);

            memcpy(&net_r, data_ptr, sizeof(uint32_t));
            data_ptr += sizeof(uint32_t);

            memcpy(&net_m, data_ptr, sizeof(uint32_t));
            data_ptr += sizeof(uint32_t);

            memcpy(&net_vx, data_ptr, sizeof(uint32_t));
            data_ptr += sizeof(uint32_t);
            memcpy(&net_vy, data_ptr, sizeof(uint32_t));
            data_ptr += sizeof(uint32_t);

            float x = network_to_float(net_x);
            float y = network_to_float(net_y);
            float r = network_to_float(net_r);
            float m = network_to_float(net_m);
            float vx = network_to_float(net_vx);
            float vy = network_to_float(net_vy);

            std::uint8_t color_r = static_cast<std::uint8_t>(*data_ptr++);
            std::uint8_t color_g = static_cast<std::uint8_t>(*data_ptr++);
            std::uint8_t color_b = static_cast<std::uint8_t>(*data_ptr++);

            sf::Color sf_color(color_r, color_g, color_b);

            // planet creation
            Planet p(r, m);
            p.setPosition(sf::Vector2f(x, y));
            p.setVelocity(sf::Vector2f(vx, vy));
            p.setColor(color_r, color_g, color_b);

            new_planets.push_back(p);

            std::vector<sf::Vertex> trajectory;
            sf::Color trailColor(color_r, color_g, color_b, 200);
            trajectory.push_back(sf::Vertex(sf::Vector2f(x, y), trailColor));
            new_trajectories.push_back(std::move(trajectory));

            std::cout << i << ") " << " x: " << x << " y: " << y << " r: " << r
                      << " m: " << m << " color: " << (int)color_r << "_"
                      << (int)color_g << "_" << (int)color_b << "\n";
          }

          std::lock_guard<std::mutex> lock(*planets_mutex);
          {
            *planets = std::move(new_planets);
          }

        } else {
          std::cerr << "Incomplete packet received: " << bytes_received
                    << " bytes, expected " << expected_size << std::endl;
        }
      }
    } else if (bytes_received < 0) {
      perror("recvfrom failed");
    }
  }
}
