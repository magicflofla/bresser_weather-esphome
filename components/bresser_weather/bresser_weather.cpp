#include "bresser_weather.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bresser_weather {

static const char *const TAG = "bresser_weather";

void BresserWeatherComponent::setup() {
    ESP_LOGI(TAG, "Initialisiere Bresser Receiver...");
    
    // Wir erzwingen die Pins: CS=15(D8), GDO0=4(D2), GDO2=5(D1)
    // Falls deine Verkabelung anders ist, musst du diese Zahlen hier ändern!
    this->ws_.begin(15, 4, 5); 
    
    ESP_LOGI(TAG, "Receiver Setup-Versuch beendet.");
}

void BresserWeatherComponent::loop() {
    // Verhindert "Too many messages queued" durch Begrenzung auf 10 Abfragen pro Sekunde
    static uint32_t last_loop = 0;
    if (millis() - last_loop < 100) {
        return;
    }
    last_loop = millis();

    int decode_status = this->ws_.getMessage();

    // Nur loggen, wenn wirklich etwas passiert, um den Puffer zu schonen
    if (decode_status == DECODE_OK) {
        const int i = 0;
        uint32_t current_id = (unsigned int)this->ws_.sensor[i].sensor_id;

        if (this->filter_enabled_ && current_id != this->filter_sensor_id_) {
            return;
        }

        if (this->temperature_sensor_ != nullptr && this->ws_.sensor[i].w.temp_ok) {
            this->temperature_sensor_->publish_state(this->ws_.sensor[i].w.temp_c);
        }
        
        // ... (andere Sensoren analog zum vorigen Code)
        
        ESP_LOGD(TAG, "Daten empfangen von ID: %08X", current_id);
        this->ws_.clearSlots();
    } 
    else if (decode_status != 0 && decode_status != 1) {
        // Logge Fehler nur im Verbose-Modus, um die Queue nicht zu füllen
        ESP_LOGV(TAG, "Radio Status: %d", decode_status);
    }
}

} // namespace bresser_weather
} // namespace esphome
