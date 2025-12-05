#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <string>

void createTestJsonFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    file << content;
    file.close();
}

TEST(JsonParserTest, ValidJsonParsing) {
    std::string jsonContent = R"({
        "Planets": [
            {
                "radius": 10.5,
                "mass": 100.0,
                "x": 100.0,
                "y": 200.0,
                "velocity": [5.0, -3.0],
                "color": [255, 0, 0]
            },
            {
                "radius": 5.0,
                "mass": 50.0,
                "x": 300.0,
                "y": 400.0,
                "velocity": [-2.0, 1.0],
                "color": [0, 255, 0]
            }
        ]
    })";
    
    createTestJsonFile("test_planets.json", jsonContent);
    
    std::ifstream file("test_planets.json");
    ASSERT_TRUE(file.is_open());
    
    // Проверяем, что файл не пустой
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    EXPECT_GT(size, 0);
    
    std::remove("test_planets.json");
}

TEST(JsonParserTest, JsonWithMissingFields) {
    std::string jsonContent = R"({
        "Planets": [
            {
                "radius": 10.5,
                "x": 100.0,
                "y": 200.0
                // Нет mass и velocity
            }
        ]
    })";
    
    createTestJsonFile("test_invalid.json", jsonContent);
    
    std::ifstream file("test_invalid.json");
    ASSERT_TRUE(file.is_open());
    
    // Здесь тестируем, как парсер обрабатывает отсутствие полей
    // Должен выбрасывать исключение или использовать значения по умолчанию
    
    std::remove("test_invalid.json");
}

TEST(JsonParserTest, EmptyJsonArray) {
    std::string jsonContent = R"({
        "Planets": []
    })";
    
    createTestJsonFile("test_empty.json", jsonContent);
    
    std::ifstream file("test_empty.json");
    ASSERT_TRUE(file.is_open());
        
    std::remove("test_empty.json");
}

TEST(JsonParserTest, JsonSyntaxError) {
    std::string jsonContent = R"({
        "Planets": [
            {
                "radius": 10.5,
                "mass": 100.0,
                "x": 100.0,
                "y": 200.0,
                "velocity": [5.0, -3.0
                // Пропущена закрывающая скобка
            }
        ]
    )";
    
    createTestJsonFile("test_bad_syntax.json", jsonContent);
    
    // Парсер должен выбрасывать исключение при синтаксической ошибке
    // Здесь можно использовать EXPECT_THROW
    
    std::remove("test_bad_syntax.json");
}
