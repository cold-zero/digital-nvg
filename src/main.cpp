#include <Arduino.h>
#include <esp_log.h>
#include <esp_camera.h>
#include <SPI.h>

static const char* TAG = "main-app";
#define OSC_DEBUG /* Comment this line to remove debug data from the screen */
#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

uint8_t *jpg_buf; /* JPEG image buffer */
size_t jpg_buf_len; /* JPEG image buffer length */
camera_fb_t *fb = NULL; /* Camera frame buffer */

#include "lgfx_config.h"

uint16_t fps = 0;
LGFX tft;

void setup()
{
    ESP_LOGI(TAG, "Initialising screen...");
    tft.begin();
    tft.setTextColor(0xFFFF, 0x0000);
    tft.setFont(&FreeMonoBoldOblique9pt7b);
    tft.fillScreen(TFT_BLACK);
#ifdef OSC_DEBUG
    tft.fillScreen(TFT_CYAN);
#endif

#ifdef OSC_DEBUG
    tft.fillScreen(TFT_PURPLE);
#endif

    ESP_LOGI(TAG, "Initializing camera...");
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
    config.pixel_format = PIXFORMAT_RGB565;
    config.frame_size = FRAMESIZE_240X240;

#ifdef OSC_DEBUG
    tft.fillScreen(TFT_YELLOW);
#endif
    ESP_LOGI(TAG, "Checking PSRAM availability...");
    if (psramFound())
    {
        ESP_LOGI(TAG, "PSRAM found!");
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.jpeg_quality = 10;
        config.fb_count = 1;
        //config.grab_mode = CAMERA_GRAB_LATEST;
    }
    else
    {
        ESP_LOGI(TAG, "PSRAM missing!");
        config.fb_location = CAMERA_FB_IN_DRAM;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

#ifdef OSC_DEBUG
    tft.fillScreen(TFT_GREEN);
#endif
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return;
    }

    /* Let's grab the sensor model and do some config on it */
    sensor_t *s = esp_camera_sensor_get();

    if (s->id.PID == OV7725_PID)
    {
        ESP_LOGI(TAG, "Using the OV7725 sensor!");
        /* TODO[coldzero]: Check if these options are OK */
        if (config.pixel_format != PIXFORMAT_RGB565)
        {
            err = esp_camera_deinit();
            if (err != ESP_OK) ESP_LOGE(TAG, "Camera de-initialisation failed!");
            config.pixel_format = PIXFORMAT_RGB565;
            err = esp_camera_init(&config);
            if (err != ESP_OK) ESP_LOGE(TAG, "Camera re-initialisation failed!");
        }
    }

    if (s->id.PID == OV2640_PID)
    {
        ESP_LOGI(TAG, "Using the OV2640 sensor!");
        /* set_pixformat doesn't seem to work so we have to
           de-init an re-init the camera to set the new format */
        if (config.pixel_format != PIXFORMAT_JPEG)
        {
            err = esp_camera_deinit();
            if (err != ESP_OK) ESP_LOGE(TAG, "Camera de-initialisation failed!");
            config.pixel_format = PIXFORMAT_JPEG;
            err = esp_camera_init(&config);
            if (err != ESP_OK) ESP_LOGE(TAG, "Camera re-initialisation failed!");
        }
    }

#ifdef OSC_DEBUG
    tft.fillScreen(TFT_ORANGE);
#endif
    ESP_LOGI(TAG, "Setup is done!");
}

void loop()
{
    unsigned long start = millis();

    tft.startWrite();

    /* Grab camera frame */
    fb = esp_camera_fb_get();
    if (!fb)
    {
        ESP_LOGE(TAG, "Failed to grab camera frame!");
    }
    else
    {
        /* Let's make sure we have a JPEG image */
        if (fb->format != PIXFORMAT_JPEG)
        {
            /* If image has a different format, let's try to convert it */
            bool jpeg_converted = frame2jpg(fb, 80, &jpg_buf, &jpg_buf_len);
            if (!jpeg_converted)
            {
                ESP_LOGE(TAG, "JPEG compression failed!");
                esp_camera_fb_return(fb);
            }
        }
        else
        {
            jpg_buf_len = fb->len;
            jpg_buf = fb->buf;
        }

        /* Draw captured image to the screen */
        tft.drawJpg(jpg_buf, jpg_buf_len, 0, 0, 240, 240);

        /* Clean up */
        if (fb->format != PIXFORMAT_JPEG)
            free(jpg_buf);
        esp_camera_fb_return(fb);
        fb = NULL;
    }

    unsigned long elapsed_time = millis() - start;
    fps = 1000 / elapsed_time;

#ifdef OSC_DEBUG /* Print on-screen debug info */
    tft.setCursor(0, 100);
    tft.printf("FPS: %d", fps);
    tft.setCursor(0, 120);
    tft.printf("Time elapsed: %lu", elapsed_time);
#endif
    tft.endWrite();
}