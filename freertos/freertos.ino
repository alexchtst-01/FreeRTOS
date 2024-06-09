#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <NewPing.h>

#define TFT_CS    5
#define TFT_RST   4
#define TFT_DC    2
#define TFT_MOSI  23
#define TFT_CLK   18
#define TFT_MISO  19
#define TFT_LED   32
#define TFT_BL    33

#define TRIG_PIN 33
#define ECHO_PIN 32
#define MAX_DISTANCE 400

NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Game constants
const int dinoWidth = 20;
const int dinoHeight = 20;
const int gravity = 1;
const int jumpStrength = -12; // Higher jump strength for the dinosaur
const int cactusWidth = 10;
int cactusSpeed = 5; // Initial cactus speed

int dinoY;
int dinoVelocity;
int cactusX;
int cactusHeight;
int score = 0;

bool dinoInAir = false;
bool passedCactus = false;

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  dinoY = tft.height() - dinoHeight;
  dinoVelocity = 0;
  cactusX = tft.width();
  cactusHeight = dinoHeight;

  // Create FreeRTOS tasks
  xTaskCreate(updateGame, "Update Game", 4096, NULL, 1, NULL);
  xTaskCreate(checkSensor, "Check Sensor", 2048, NULL, 1, NULL);
}

void loop() {
  // Empty loop as tasks are handled by FreeRTOS
}

void updateGame(void *pvParameters) {
  while (true) {
    tft.fillScreen(ILI9341_WHITE);

    // Update dinosaur position
    dinoY += dinoVelocity;
    dinoVelocity += gravity;

    // Prevent dinosaur from going off the ground
    if (dinoY > tft.height() - dinoHeight) {
      dinoY = tft.height() - dinoHeight;
      dinoVelocity = 0;
      dinoInAir = false; // Dinosaur has landed
    }

    // Draw dinosaur
    tft.fillRect(30, dinoY, dinoWidth, dinoHeight, ILI9341_BLACK);

    // Update cactus position
    cactusX -= cactusSpeed;
    if (cactusX < -cactusWidth) {
      cactusX = tft.width();
      cactusHeight = dinoHeight;
      passedCactus = false; // Reset flag for new cactus
    }

    // Draw cactus
    tft.fillRect(cactusX, tft.height() - cactusHeight, cactusWidth, cactusHeight, ILI9341_GREEN);

    // Check for collisions
    if (30 + dinoWidth > cactusX && 30 < cactusX + cactusWidth) {
      if (dinoY + dinoHeight > tft.height() - cactusHeight) {
        tft.fillScreen(ILI9341_RED);
        tft.setCursor(tft.width() / 2 - 50, tft.height() / 2 - 10);
        tft.print("Permainan Selesai");
        tft.setCursor(tft.width() / 2 - 50, tft.height() / 2 + 10);
        tft.print("Score: ");
        tft.print(score);
        vTaskDelete(NULL);
      }
    }

    // Update score and increase cactus speed
    if (cactusX + cactusWidth < 30 && !passedCactus) {
      score++;
      cactusSpeed = 5 + score / 2; // Increase speed every 5 points
      passedCactus = true; // Mark this cactus as passed
    }

    // Display score
    tft.setCursor(10, 10); // Display score in the top-left corner
    tft.print("Score: ");
    tft.print(score);

    vTaskDelay(30 / portTICK_PERIOD_MS); // 30 ms delay
  }
}

void checkSensor(void *pvParameters) {
  while (true) {
    unsigned int distance = sonar.ping_cm();

    if (distance <= 3 && !dinoInAir) {
      dinoVelocity = jumpStrength;
      dinoInAir = true;
    }

    Serial.print("Distance: ");
    if (distance == 0) {
      Serial.println("Out of range");
    } else {
      Serial.print(distance);
      Serial.println(" cm");
    }

    vTaskDelay(50 / portTICK_PERIOD_MS); // 50 ms delay
  }
}