#include <Arduino.h>
#include <esp_camera.h>
#include <SPI.h>

#define CAMERA_MODEL_AI_THINKER

#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include "camera_pins.h"

#define DEBUG

uint16_t fps = 0;

TFT_eSPI tft = TFT_eSPI();

bool render_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
    if (y >= tft.height())
        return 0;

    tft.pushImage(x, y, w, h, bitmap);

    // Return 1 to decode next block
    return 1;
}

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    Serial.println("Initialising screen...");
    tft.begin();
    tft.setRotation(0);
    tft.setTextColor(0xFFFF, 0x0000);
    tft.setFreeFont(&FreeMonoBoldOblique9pt7b);
    tft.fillScreen(TFT_BLACK);
#ifdef DEBUG
    tft.fillScreen(TFT_CYAN);
#endif

    Serial.println("Initializing JPEG decode library...");
    TJpgDec.setJpgScale(1);
    TJpgDec.setSwapBytes(true);
    TJpgDec.setCallback(render_output);
#ifdef DEBUG
    tft.fillScreen(TFT_PURPLE);
#endif

    Serial.println("Initializing camera...");
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

#ifdef DEBUG
    tft.fillScreen(TFT_YELLOW);
#endif
    Serial.print("Checking PSRAM availability => ");
    if (psramFound())
    {
        Serial.println("PSRAM found!");
        config.frame_size = FRAMESIZE_QVGA; /* 320x240 */
        config.jpeg_quality = 10;
        config.fb_count = 2;
    }
    else
    {
        Serial.println("PSRAM missing!");
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

#ifdef DEBUG
    tft.fillScreen(TFT_GREEN);
#endif
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

#ifdef DEBUG
    tft.fillScreen(TFT_ORANGE);
#endif
    Serial.println("Setup is done!");
}

void loop()
{
    unsigned long start = millis();

    camera_fb_t *fb = NULL;
    fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("Failed to grab camera frame!");
    }
    else
    {
        TJpgDec.drawJpg(0, 0, (const uint8_t*)fb->buf, fb->len);
        esp_camera_fb_return(fb);
        fb = NULL;
    }

    unsigned long elapsed_time = millis() - start;
    fps = 1000 / elapsed_time;

#ifdef DEBUG
    tft.setCursor(0, 100);
    tft.printf("FPS: %d", fps);
    tft.setCursor(0, 120);
    tft.printf("Time elapsed: %lu", elapsed_time);
#endif
}