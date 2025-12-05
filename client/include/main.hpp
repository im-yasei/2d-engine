#pragma once

#include "engine.hpp"
#include "json/json.h"
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>

#include <arpa/inet.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define PORT 8080
#define MAXLINE 1024
#define JSON_USE_STRING_VIEW 0

#ifdef __linux__
int flags = MSG_CONFIRM;
#elif __APPLE__
int flags = 0; // macOS doesn't support MSG_CONFIRM
#else
int flags = 0;
#endif

extern std::atomic<bool> clientRunning;



// ==================== ПАРСИНГ JSON ====================
std::vector<Planet> loadPlanetsFromJson(const std::string& filename);

// ==================== ВИЗУАЛИЗАЦИЯ ====================
void initializeWindow(sf::RenderWindow& window);
void handleCameraMovement(sf::View& camera, sf::RenderWindow& window, 
                         float cameraSpeed, int scrollBorder);
void drawTrajectories(sf::RenderWindow& window, 
                     const std::vector<Planet>& planets,
                     std::vector<std::vector<sf::Vertex>>& trajectories);

// ==================== СЕТЕВАЯ ЧАСТЬ ====================
void client_send(int sockfd, struct sockaddr_in *servaddr,
                 std::vector<Planet> *planets, std::mutex *planets_mutex);
void client_receive(int sockfd, char *buffer, struct sockaddr_in *servaddr);
int createSocket();
bool setupSocket(int sockfd, const std::string& server_ip = "127.0.0.1");
void cleanupSocket(int sockfd, std::thread& receive_thread, std::thread& send_thread);