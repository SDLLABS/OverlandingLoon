// Car Inclinometer + barometer + optional GNSS for XIAO ESP32-S3 / Round Display 104030087
// Port: preserve the original LazyDays Creation LVGL UI and sensor logic.
// Original license and attribution: LICENSE.md
// An open source project by LazyDays Creation https://www.facebook.com/lazydayyt

#include <Arduino.h>
#include <lvgl.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP3XX.h>
#include <math.h>

// 0: BMP390 only; 1: BMP390 + optional SparkFun u-blox M10 over I2C (0x42).
// GNSS mode requires the "SparkFun u-blox GNSS v3" Arduino library.
#define USE_GNSS_I2C 0
#if USE_GNSS_I2C
#include <SparkFun_u-blox_GNSS_v3.h>
#endif

#include <LovyanGFX.hpp>
#include "LGFX_GC9A01.hpp"

extern "C"
{
#include "ui.h"
#include "ui_Screen1.h"
}

// XIAO pin names (not raw ESP32-S3 GPIO numbers):
#define PIN_BL       D6 // Backlight (GPIO43) - KE switch must enable D6
#define PIN_SDA      D4 // Shared I2C with CHSC6X/RTC/MPU6050 (GPIO5)
#define PIN_SCL      D5 // Shared I2C (GPIO6)
#define PIN_TOUCH_INT D7 // CHSC6X touch interrupt (GPIO44)
#define PIN_SD_CS    D2 // Keep onboard microSD deselected (GPIO3)
#define TOUCH_ADDR   0x2E


// ---------------- Display (GC9A01, config in LGFX_GC9A01.hpp) ----------------
LGFX display;

bool pageToggleRequested = false;
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[screenWidth * 20];
static lv_color_t buf2[screenWidth * 20];

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    display.startWrite();
    display.setAddrWindow(area->x1, area->y1, w, h);
    display.pushImage(area->x1, area->y1, w, h, (uint16_t *)&color_p->full);
    display.endWrite();

    lv_disp_flush_ready(disp);
}

// The original project ignored touch. Enable the CHSC6X driver here;
// the exported UI has no buttons, so touches do not change measurements.
// Decode is based on Seeed_Arduino_RoundDisplay's documented 5-byte report.
void my_touchpad_read(lv_indev_drv_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->state = LV_INDEV_STATE_RELEASED;
    if (digitalRead(PIN_TOUCH_INT) != LOW) return;

    uint8_t packet[5] = {0};
    int received = Wire.requestFrom((uint8_t)TOUCH_ADDR, (uint8_t)5);
    if (received != 5) {
        while (Wire.available()) (void)Wire.read();
        return;
    }
    for (uint8_t i = 0; i < 5; ++i) packet[i] = (uint8_t)Wire.read();
    if (packet[0] != 1) return;
    const uint16_t x = packet[2];
    const uint16_t y = packet[4];
    if (x >= screenWidth || y >= screenHeight) return;
    data->point.x = (lv_coord_t)x;
    data->point.y = (lv_coord_t)y;
    data->state = LV_INDEV_STATE_PRESSED;
    // Screen changes are performed in loop(), not inside the LVGL input callback.
    static uint32_t lastTapMs = 0;
    uint32_t touchNow = millis();
    if ((uint32_t)(touchNow - lastTapMs) > 550) {
        lastTapMs = touchNow;
        pageToggleRequested = true;
    }
}

// ---------------- Inclinometer UI ----------------
// Gauge: 0 at top, clockwise. Roll zero at 270 deg (left), pitch zero at 90 deg (right).
// +-40 deg of tilt spans 54 deg of gauge arc -> 1.35 gauge deg per tilt deg.
static const float MAX_TILT = 40.0f;
static const float GAUGE_DEG_PER_DEG = 1.35f;

// Car icon rotation: flip the sign if an icon tilts the wrong way
static const float ROLL_ICON_SIGN = 1.0f;
static const float PITCH_ICON_SIGN = -1.0f;

static int32_t lastRollAng = -1, lastPitchAng = -1;
static int32_t lastRollIcon = -1, lastPitchIcon = -1;
static int lastRollLbl = -1000, lastPitchLbl = -1000;

static inline int32_t wrapAngle(float deg) // deg -> LVGL 0.1 deg units, 0..3599
{
    int32_t a = (int32_t)lroundf(deg * 10.0f) % 3600;
    if (a < 0) a += 3600;
    return a;
}

// pitch: deg, + = nose up. roll: deg, + = right side down (clockwise)
static void setInclination(float pitch, float roll)
{
    // Labels (true values): pitch with sign, roll without
    int rl = (int)lroundf(fabsf(roll));
    int pl = (int)lroundf(pitch);
    if (rl != lastRollLbl)
    {
        lastRollLbl = rl;
        lv_label_set_text_fmt(ui_labelroll, "%d", rl);
    }
    if (pl != lastPitchLbl)
    {
        lastPitchLbl = pl;
        if (pl == 0) lv_label_set_text(ui_labelpitch, "0");
        else lv_label_set_text_fmt(ui_labelpitch, "%+d", pl);
    }

    // Pointers and icons capped at +-40
    if (pitch > MAX_TILT) pitch = MAX_TILT;
    if (pitch < -MAX_TILT) pitch = -MAX_TILT;
    if (roll > MAX_TILT) roll = MAX_TILT;
    if (roll < -MAX_TILT) roll = -MAX_TILT;

    // Roll pointer: clockwise roll -> clockwise rotation
    int32_t rollAng = wrapAngle(roll * GAUGE_DEG_PER_DEG);
    if (rollAng != lastRollAng)
    {
        lastRollAng = rollAng;
        lv_img_set_angle(ui_rollpointer, rollAng);
    }

    // Pitch pointer: +pitch moves toward the top (counter-clockwise on the right side)
    int32_t pitchAng = wrapAngle(-pitch * GAUGE_DEG_PER_DEG);
    if (pitchAng != lastPitchAng)
    {
        lastPitchAng = pitchAng;
        lv_img_set_angle(ui_pitchpointer, pitchAng);
    }

    // Car icons
    int32_t rollIcon = wrapAngle(roll * ROLL_ICON_SIGN);
    if (rollIcon != lastRollIcon)
    {
        lastRollIcon = rollIcon;
        lv_img_set_angle(ui_rollicon, rollIcon);
    }
    int32_t pitchIcon = wrapAngle(pitch * PITCH_ICON_SIGN);
    if (pitchIcon != lastPitchIcon)
    {
        lastPitchIcon = pitchIcon;
        lv_img_set_angle(ui_pitchicon, pitchIcon);
    }
}

// ---------------- MPU6050 ----------------
// Flip these if the display moves the wrong way for your mounting
//  Disclaimer : This project is part of a cockpit dashboard for an FPV RC plane. It is a hobby build for unmanned model aircraft only.

static const float ROLL_SIGN = 1.0f;
static const float PITCH_SIGN = -1.0f;

static const float ALPHA = 0.98f; // complementary filter: gyro weight (higher = smoother, slower drift correction)

Adafruit_MPU6050 mpu;
static bool sensorOk = false;
static float gyroBiasX = 0, gyroBiasY = 0; // rad/s
static float rollOffset = 0, pitchOffset = 0; // accel angle offsets from startup calibration
static float rollDeg = 0, pitchDeg = 0;

static void accelAngles(const sensors_event_t &a, float &roll, float &pitch)
{
    float ax = a.acceleration.x, ay = a.acceleration.y, az = a.acceleration.z;
    roll = atan2f(ay, az) * RAD_TO_DEG - rollOffset;
    pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG - pitchOffset;
}

// Keep the board still: averages gyro bias and sets the starting angles from gravity
static void calibrateSensor()
{
    const int N = 200;
    float sgx = 0, sgy = 0, sr = 0, sp = 0;
    for (int i = 0; i < N; i++)
    {
        sensors_event_t a, g, t;
        mpu.getEvent(&a, &g, &t);
        float r, p;
        accelAngles(a, r, p);
        sgx += g.gyro.x;
        sgy += g.gyro.y;
        sr += r;
        sp += p;
        delay(5);
    }
    gyroBiasX = sgx / N;
    gyroBiasY = sgy / N;
    rollOffset = sr / N;   // whatever the board reads at boot becomes "zero"
    pitchOffset = sp / N;
    rollDeg = 0;
    pitchDeg = 0;
}

// ~100 Hz, non-blocking
static void updateSensor()
{
    static uint32_t lastUs = 0;
    uint32_t nowUs = micros();
    if (lastUs == 0) { lastUs = nowUs; return; }

    float dt = (nowUs - lastUs) * 1e-6f;
    if (dt < 0.01f) return;
    lastUs = nowUs;
    if (dt > 0.1f) dt = 0.1f;

    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);

    float accRoll, accPitch;
    accelAngles(a, accRoll, accPitch);

    float gx = (g.gyro.x - gyroBiasX) * RAD_TO_DEG; // roll rate
    float gy = (g.gyro.y - gyroBiasY) * RAD_TO_DEG; // pitch rate

    rollDeg = ALPHA * (rollDeg + gx * dt) + (1.0f - ALPHA) * accRoll;
    pitchDeg = ALPHA * (pitchDeg + gy * dt) + (1.0f - ALPHA) * accPitch;
}


// ---------------- Barometer / altimeter / optional GNSS ----------------
// BMP390 shares D4/D5 with MPU6050, CHSC6X and RTC.
// This is AMBIENT pressure; QNH is a model-based estimate from GNSS altitude.
// A GNSS altitude is MSL (not ellipsoid height); reject solutions with poor vertical accuracy.
static constexpr uint8_t BMP3_I2C_ADDRESS = 0x77; // Set 0x76 if your breakout straps SDO low.
static constexpr float DEFAULT_QNH_HPA = 1013.25f; // Standard-atmosphere assumption, NOT local QNH.
static constexpr float MAX_GNSS_VERTICAL_ERROR_M = 8.0f; // Receiver-reported uncertainty, not a guarantee.
static Adafruit_BMP3XX baro;
static bool baroOk = false;
static float ambientPressureHpa = NAN;
static float ambientTemperatureC = NAN;
static float altitudeMeters = NAN;
static float qnhHpa = DEFAULT_QNH_HPA;
static bool qnhFromGnss = false;
static uint8_t goodGnssSamples = 0;
static float runningQnh = 0.0f;

#if USE_GNSS_I2C
static SFE_UBLOX_GNSS gnss;
static bool gnssOk = false;
static bool gnssFixGood = false;
static float gnssAltMeters = NAN;
static float gnssVAccMeters = NAN;
static uint32_t lastGoodFixMs = 0;
#endif

static lv_obj_t *envScreen = nullptr;
static lv_obj_t *pressureValueLabel = nullptr;
static lv_obj_t *altitudeValueLabel = nullptr;
static lv_obj_t *gnssValueLabel = nullptr;
static lv_obj_t *qnhValueLabel = nullptr;
static lv_obj_t *referenceLabel = nullptr;
static bool environmentalPage = false;

static lv_obj_t *envLabel(const char *txt, int16_t y, uint32_t color, bool numeric = false)
{
    lv_obj_t *label = lv_label_create(envScreen);
    lv_obj_set_width(label, 210);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_label_set_text(label, txt);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    if (numeric) lv_obj_set_style_text_font(label, &ui_font_Font1, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, y);
    return label;
}

static void initEnvironmentPage()
{
    envScreen = lv_obj_create(nullptr);
    lv_obj_clear_flag(envScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(envScreen, lv_color_hex(0x030A12), 0);
    lv_obj_set_style_bg_opa(envScreen, LV_OPA_COVER, 0);
    envLabel("BAROMETRO", -94, 0x5CC9F5);
    envLabel("PRESSAO ATMOSFERICA", -72, 0xB6CBDC);
    pressureValueLabel = envLabel("--.- hPa", -49, 0xFFFFFF);
    envLabel("ALTITUDE ESTIMADA", -23, 0xB6CBDC);
    altitudeValueLabel = envLabel("---- m", 1, 0xFFFFFF);
    referenceLabel = envLabel("REF: 1013.25 hPa (padrao)", 29, 0xF7D17A);
    gnssValueLabel = envLabel("GNSS: desativado", 50, 0x9DC7B3);
    qnhValueLabel = envLabel("QNH: 1013.3 hPa (estimado)", 70, 0xA9BAC9);
    envLabel("TOQUE PARA VOLTAR", 92, 0x5CC9F5);
}

static void updateEnvironmentPage()
{
    char buffer[52];
    if (baroOk && isfinite(ambientPressureHpa)) {
        snprintf(buffer, sizeof(buffer), "%.1f hPa", ambientPressureHpa);
        lv_label_set_text(pressureValueLabel, buffer);
        snprintf(buffer, sizeof(buffer), "%.0f m", altitudeMeters);
        lv_label_set_text(altitudeValueLabel, buffer);
    } else {
        lv_label_set_text(pressureValueLabel, "SENSOR AUSENTE");
        lv_label_set_text(altitudeValueLabel, "-- m");
    }
    lv_label_set_text(referenceLabel,
        qnhFromGnss ? "REF: corrigida por GNSS" : "REF: 1013.25 hPa (padrao)");
#if USE_GNSS_I2C
    if (gnssOk && gnssFixGood) {
        snprintf(buffer, sizeof(buffer), "GNSS: %.0f m (vAcc %.1f m)", gnssAltMeters, gnssVAccMeters);
    } else if (gnssOk) {
        snprintf(buffer, sizeof(buffer), "GNSS: sem fix fiavel");
    } else {
        snprintf(buffer, sizeof(buffer), "GNSS: nao encontrado");
    }
#else
    snprintf(buffer, sizeof(buffer), "GNSS: opcional (desativado)");
#endif
    lv_label_set_text(gnssValueLabel, buffer);
    snprintf(buffer, sizeof(buffer), "QNH: %.1f hPa%s", qnhHpa, qnhFromGnss ? "" : " (padrao)");
    lv_label_set_text(qnhValueLabel, buffer);
}

static void updateBarometer()
{
    if (!baroOk) return;
    if (!baro.performReading()) return;
    float p = (float)(baro.pressure / 100.0); // Adafruit returns Pa, screen uses hPa.
    if (!isfinite(p) || p < 300.0f || p > 1250.0f) return;
    ambientPressureHpa = p;
    ambientTemperatureC = (float)baro.temperature;
    // International standard atmosphere approximation: calibrated sea-level pressure required
    // for useful absolute altitude. Relative changes are meaningful even without GNSS.
    altitudeMeters = 44330.0f * (1.0f - powf(p / qnhHpa, 0.19029495f));
}

#if USE_GNSS_I2C
static void updateGnss()
{
    if (!gnssOk) return;
    // setAutoPVT(true) makes getPVT non-blocking in normal operation.
    // Read once per second to avoid delaying the 100 Hz inclinometer.
    bool received = gnss.getPVT(20);
    if (received) {
        const float alt = gnss.getAltitudeMSL() * 0.001f;
        const float acc = gnss.getVerticalAccEst() * 0.001f;
        const uint8_t fix = gnss.getFixType();
        gnssFixGood = gnss.getGnssFixOk() && (fix >= 3) &&
                      isfinite(alt) && (alt > -500.0f) && (alt < 5000.0f) &&
                      isfinite(acc) && (acc > 0.0f) && (acc <= MAX_GNSS_VERTICAL_ERROR_M);
        if (gnssFixGood) {
            gnssAltMeters = alt;
            gnssVAccMeters = acc;
            lastGoodFixMs = millis();
            if (baroOk && isfinite(ambientPressureHpa)) {
                // QNH estimate in a standard atmosphere. Limit outlier propagation.
                const float ratio = 1.0f - alt / 44330.0f;
                const float newQnh = ambientPressureHpa / powf(ratio, 5.255f);
                if (isfinite(newQnh) && newQnh > 870.0f && newQnh < 1090.0f) {
                    if (!qnhFromGnss) {
                        runningQnh += newQnh;
                        goodGnssSamples++;
                        if (goodGnssSamples >= 8) {
                            qnhHpa = runningQnh / goodGnssSamples;
                            qnhFromGnss = true;
                        }
                    } else {
                        // Very slow correction: GNSS vertical noise must not enter the display.
                        float step = 0.005f * (newQnh - qnhHpa);
                        step = constrain(step, -0.03f, 0.03f);
                        qnhHpa += step;
                    }
                }
            }
        }
    }
    // Invalidating stale data prevents the UI showing an old GNSS fix as current.
    if (millis() - lastGoodFixMs > 3500) gnssFixGood = false;
}
#endif

static void togglePage()
{
    environmentalPage = !environmentalPage;
    lv_scr_load(environmentalPage ? envScreen : ui_Screen1);
}


void setup()
{
    Serial.begin(115200);
    delay(500);

    // All I2C devices share D4/D5. Configure before LVGL touch reads.
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);
    pinMode(PIN_TOUCH_INT, INPUT_PULLUP);

    // Backlight is gated by the Round Display KE switch (ON side).
    pinMode(PIN_BL, OUTPUT);
    digitalWrite(PIN_BL, HIGH);
    // Shared SPI bus: keep microSD card deselected unless used.
    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);

    display.init();
    display.setRotation(0);
    display.setSwapBytes(true);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, screenWidth * 20);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    ui_init();
    initEnvironmentPage();
    updateEnvironmentPage();
    setInclination(0, 0);

    // The barometer is optional in hardware, but its Arduino library must be installed.
    baroOk = baro.begin_I2C(BMP3_I2C_ADDRESS, &Wire);
    if (baroOk) {
        baro.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
        baro.setPressureOversampling(BMP3_OVERSAMPLING_4X);
        baro.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
        baro.setOutputDataRate(BMP3_ODR_25_HZ);
        Serial.println("BMP390/BMP388 initialized");
    } else {
        Serial.println("BMP3XX not found - check address 0x77/0x76 and wiring");
    }
#if USE_GNSS_I2C
    gnssOk = gnss.begin(Wire);
    if (gnssOk) {
        gnss.setAutoPVT(true);
        Serial.println("u-blox GNSS found over I2C");
    } else {
        Serial.println("u-blox GNSS I2C not found (0x42)");
    }
#endif

    if (mpu.begin(0x68, &Wire))
    {
        Wire.setClock(400000);
        mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);
        delay(100);
        calibrateSensor(); // keep the board still for ~1 s
        sensorOk = true;
    }
    else
    {
        Serial.println("MPU6050 not found!");
    }

    Serial.printf("Free heap: %u, largest block: %u\n",
                  (unsigned)ESP.getFreeHeap(),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

void loop()
{
    static uint32_t lastTick = 0;
    static uint32_t lastUpdate = 0;
    static uint32_t lastBaro = 0;
    static uint32_t lastGnss = 0;
    static uint32_t lastEnvUI = 0;

    uint32_t now = millis();
    lv_tick_inc(now - lastTick);
    lastTick = now;

    if (sensorOk) updateSensor();
    if (now - lastBaro >= 250) {
        lastBaro = now;
        updateBarometer();
    }
#if USE_GNSS_I2C
    if (now - lastGnss >= 1000) {
        lastGnss = now;
        updateGnss();
    }
#endif
    if (now - lastEnvUI >= 500) {
        lastEnvUI = now;
        updateEnvironmentPage();
    }
    if (pageToggleRequested) {
        pageToggleRequested = false;
        togglePage();
    }

    if (now - lastUpdate >= 20)
    {
        lastUpdate = now;
        if (sensorOk) setInclination(pitchDeg * PITCH_SIGN, rollDeg * ROLL_SIGN);
    }
    static uint32_t lastPrint = 0; // Serial debug
    if (sensorOk && now - lastPrint >= 100)
    {
        lastPrint = now;
        Serial.printf("pitch: %.1f  roll: %.1f\n", pitchDeg, rollDeg);
    }
    lv_timer_handler();
    delay(1); // Yield to FreeRTOS / avoid busy-looping
}
