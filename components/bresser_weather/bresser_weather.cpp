#include "bresser_weather.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace bresser_weather
    {
        static const char *const TAG = "bresser_weather";

        void BresserWeatherComponent::setup()
        {
            ESP_LOGI(TAG, "Starte Bresser Weather Sensor Receiver...");
            // Initialisierung des CC1101 über die Library
            this->ws_.begin(); 
            ESP_LOGI(TAG, "Receiver Initialisierung abgeschlossen.");
        }

        void BresserWeatherComponent::loop()
        {
            // Versuche eine Nachricht zu empfangen
            int decode_status = this->ws_.getMessage();

            // Logge den Status, wenn er nicht "DATA_INVALID" oder "0" ist, 
            // um zu sehen ob überhaupt Signale erkannt werden
            if (decode_status != 0 && decode_status != DECODE_OK) {
                ESP_LOGV(TAG, "Funk-Status erkannt: %d (Kein gültiges Paket)", decode_status);
            }

            if (decode_status == DECODE_OK)
            {
                // In der Regel nutzen wir den ersten gefundenen Slot
                const int i = 0;
                uint32_t current_id = (unsigned int)this->ws_.sensor[i].sensor_id;

                ESP_LOGI(TAG, "Paket empfangen! Sensor-ID: %08X, Typ: %d", current_id, this->ws_.sensor[i].s_type);

                // Filter-Check
                if (this->filter_enabled_ && current_id != this->filter_sensor_id_)
                {
                    ESP_LOGD(TAG, "Ignoriere Sensor-ID %08X (Filter ist aktiv für: %08X)",
                             current_id, (unsigned int)this->filter_sensor_id_);
                    return;
                }

                // Prüfe auf gültige Wetterdaten-Typen
                if ((this->ws_.sensor[i].s_type == SENSOR_TYPE_WEATHER0) ||
                    (this->ws_.sensor[i].s_type == SENSOR_TYPE_WEATHER1) ||
                    (this->ws_.sensor[i].s_type == SENSOR_TYPE_WEATHER3) ||
                    (this->ws_.sensor[i].s_type == SENSOR_TYPE_WEATHER8))
                {
                    // Sensordaten veröffentlichen
                    if (this->sensor_id_sensor_ != nullptr) {
                        char id_str[16];
                        snprintf(id_str, sizeof(id_str), "%08X", current_id);
                        this->sensor_id_sensor_->publish_state(id_str);
                    }

                    if (this->rssi_sensor_ != nullptr) {
                        this->rssi_sensor_->publish_state(this->ws_.sensor[i].rssi);
                    }

                    if (this->battery_sensor_ != nullptr) {
                        // Invertierte Logik für HA: ON = Low Battery
                        this->battery_sensor_->publish_state(!this->ws_.sensor[i].battery_ok);
                    }

                    if (this->ws_.sensor[i].w.temp_ok && this->temperature_sensor_ != nullptr) {
                        this->temperature_sensor_->publish_state(this->ws_.sensor[i].w.temp_c);
                    }

                    if (this->ws_.sensor[i].w.humidity_ok && this->humidity_sensor_ != nullptr) {
                        this->humidity_sensor_->publish_state(this->ws_.sensor[i].w.humidity);
                    }

                    if (this->ws_.sensor[i].w.wind_ok) {
                        if (this->wind_gust_sensor_ != nullptr)
                            this->wind_gust_sensor_->publish_state(this->ws_.sensor[i].w.wind_gust_meter_sec);
                        if (this->wind_speed_sensor_ != nullptr)
                            this->wind_speed_sensor_->publish_state(this->ws_.sensor[i].w.wind_avg_meter_sec);
                        if (this->wind_direction_sensor_ != nullptr)
                            this->wind_direction_sensor_->publish_state(this->ws_.sensor[i].w
