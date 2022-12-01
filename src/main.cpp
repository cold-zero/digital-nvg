#include <Arduino.h>
#include <esp_camera.h>
#include <soc/soc.h>           // Disable brownour problems
#include <soc/rtc_cntl_reg.h>  // Disable brownour problems
#include <driver/rtc_io.h>

#include <SPI.h>
#include <TFT_eSPI.h>
#include <TFT_eFEX.h>

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

//#define DEBUG /* Comment this line to remove debug data from the screen */
#ifdef DEBUG
#define LOG(x) Serial.printf(x)
#else
#define LOG(X)
#endif

TFT_eSPI tft = TFT_eSPI();
TFT_eFEX  fex = TFT_eFEX(&tft);

camera_fb_t* fb = NULL;
uint32_t fps = 0;

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

#ifdef DEBUG
    Serial.begin(115200);
    Serial.setDebugOutput(true);
#endif
    LOG("\n");
    LOG("Initialising screen...");
    tft.begin();
    tft.setRotation(0);  // 0 - left; 2 - right
    tft.setTextColor(0xFFFF, 0x0000);
    tft.setFreeFont(&FreeMonoBoldOblique9pt7b);
    tft.fillScreen(TFT_BLACK);
#ifdef DEBUG
    tft.fillScreen(TFT_CYAN);
#endif
    LOG("Initializing camera...");
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
    LOG("Checking PSRAM availability => ");
    if(psramFound())
    {
        LOG("PSRAM found!");
        config.frame_size = FRAMESIZE_QVGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
        config.jpeg_quality = 10;
        config.grab_mode = CAMERA_GRAB_LATEST;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.fb_count = 2;
    }
    else
    {
        LOG("PSARM missing!");
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
        config.fb_location = CAMERA_FB_IN_DRAM;
        config.fb_count = 1;
    }

#ifdef DEBUG
    tft.fillScreen(TFT_GREEN);
#endif
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        LOG("Camera init failed!\n");
        return;
    }

#ifdef DEBUG
    tft.fillScreen(TFT_ORANGE);
#endif
    LOG("Setup is done!");
}

void loop()
{
    uint32_t start = millis();

    fb = esp_camera_fb_get();

    if (!fb)
    {
        LOG("Failed to grab camera frame!");
        return;
    }
    else
    {
        fex.drawJpg((const uint8_t*)fb->buf, fb->len, 0, 0, 240, 240);
    }

    esp_camera_fb_return(fb);
    fb = NULL;

    uint32_t elapsed_time = millis() - start;
    fps = 1000 / elapsed_time;

#ifdef DEBUG
    tft.setCursor(0, 100);
    tft.printf("FPS: %d", fps);
    tft.setCursor(0, 120);
    tft.printf("Time elapsed: %lu", elapsed_time);
#endif
}