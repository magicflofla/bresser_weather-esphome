#include "bresser_weather.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bresser_weather {

static const char *const TAG = "bresser_weather";

void BresserWeatherComponent::setup() {
    ESP_LOGI(TAG, "Starte Bresser Weather Sensor Receiver...");
    this->ws_.begin();
    ESP_LOGI(TAG, "Receiver Initialisierung abgeschlossen.");
}

void BresserWeatherComponent::loop() {
    // Führe die Abfrage nur alle 100ms aus, um die CPU und Logs zu entlasten
    static uint32_t last_loop = 0;
    if (millis() - last_loop < 100) {
        return;
    }
    last_loop = millis();

    int decode_status = this->ws_.getMessage();

    if (decode_status == DECODE_OK) {
        const int i = 0;
        uint32_t current_id = (unsigned int)this->ws_.sensor[i].sensor_id;

        ESP_LOGI(TAG, "Paket empfangen! Sensor-ID: %08X, Typ: %d", current_id, this->ws_.sensor[i].s_type);

        if (this->filter_enabled_ && current_id != this->filter_sensor_id_) {
            ESP_LOGD(TAG, "Ignoriere Sensor-ID %08X (Filter aktiv für: %08X)", current_id, (unsigned int)this->filter_sensor_id_);
            return;
        }

        if ((this->ws_.sensor[i].s_type == SENSOR_TYPE_WEATHER0) ||
            (this->ws_.sensor[i].s_type == SENSOR_TYPE_WEATHER1) ||
            (this->ws_.sensor[i].s_type == SENSOR_TYPE_WEATHER3) ||
            (this->ws_.sensor[i].s_type == SENSOR_TYPE_WEATHER8)) {

            if (this->sensor_id_sensor_ != nullptr) {
                char id_str[16];
                snprintf(id_str, sizeof(id_str), "%08X", current_id);
                this->sensor_id_sensor_->publish_state(id_str);
            }

            if (this->rssi_sensor_ != nullptr) {
                this->rssi_sensor_->publish_state(this->ws_.sensor[i].rssi);
            }

            if (this->battery_sensor_ != nullptr) {
                // ON = Low Battery (HA Standard)
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
                    this->wind_direction_sensor_->publish_state(this->ws_.sensor[i].w.wind_direction_deg);
            }

            if (this->ws_.sensor[i].w.rain_ok && this->rain_sensor_ != nullptr) {
                this->rain_sensor_->publish_state(this->ws_.sensor[i].w.rain_mm);
            }

            ESP_LOGD(TAG, "Daten verarbeitet: ID=%08X, Temp=%.1f, RSSI=%.1f", current_id, this->ws_.sensor[i].w.temp_c, this->ws_.sensor[i].rssi);
        }
        this->ws_.clearSlots();
    }
}

} // namespace bresser_weather
} // namespace esphome
