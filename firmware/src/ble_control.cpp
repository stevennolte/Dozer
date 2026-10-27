#include "ble_control.h"

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "protocol.h"

static volatile bool connected = false;
static volatile uint32_t last_drive_ms = 0;
static volatile uint32_t last_pins_ms = 0;
static volatile int8_t cmd_left = 0;
static volatile int8_t cmd_right = 0;
static volatile uint8_t cmd_pins[4] = {0, 0, 0, 0};

static void apply_drive(int8_t left, int8_t right) {
    cmd_left = left;
    cmd_right = right;
    last_drive_ms = millis();
}

static void apply_pins(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4) {
    cmd_pins[0] = in1;
    cmd_pins[1] = in2;
    cmd_pins[2] = in3;
    cmd_pins[3] = in4;
    last_pins_ms = millis();
}

static void apply_stop() {
    cmd_left = 0;
    cmd_right = 0;
    cmd_pins[0] = cmd_pins[1] = cmd_pins[2] = cmd_pins[3] = 0;
    last_drive_ms = millis();
    last_pins_ms = millis();
}

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer *server) override {
        (void)server;
        connected = true;
        cmd_left = 0;
        cmd_right = 0;
        cmd_pins[0] = cmd_pins[1] = cmd_pins[2] = cmd_pins[3] = 0;
        last_drive_ms = 0;
        last_pins_ms = 0;
        Serial.println("[ble] connected");
    }

    void onDisconnect(NimBLEServer *server) override {
        connected = false;
        cmd_left = 0;
        cmd_right = 0;
        cmd_pins[0] = cmd_pins[1] = cmd_pins[2] = cmd_pins[3] = 0;
        last_drive_ms = 0;
        last_pins_ms = 0;
        Serial.println("[ble] disconnected, advertising");
        server->startAdvertising();
    }
};

class DriveCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *characteristic) override {
        std::string value = characteristic->getValue();
        if (value.size() < 2) {
            Serial.printf("[ble] drive packet too short (%u bytes)\n",
                          static_cast<unsigned>(value.size()));
            return;
        }
        const auto *bytes = reinterpret_cast<const uint8_t *>(value.data());
        apply_drive(static_cast<int8_t>(bytes[0]), static_cast<int8_t>(bytes[1]));
    }
};

class PinsCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *characteristic) override {
        std::string value = characteristic->getValue();
        if (value.size() < 4) {
            Serial.printf("[ble] pins packet too short (%u bytes)\n",
                          static_cast<unsigned>(value.size()));
            return;
        }
        const auto *bytes = reinterpret_cast<const uint8_t *>(value.data());
        apply_pins(bytes[0], bytes[1], bytes[2], bytes[3]);
    }
};

class StopCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *characteristic) override {
        (void)characteristic;
        Serial.println("[ble] stop");
        apply_stop();
    }
};

static ServerCallbacks server_callbacks;
static DriveCallbacks drive_callbacks;
static PinsCallbacks pins_callbacks;
static StopCallbacks stop_callbacks;

void ble_begin() {
    NimBLEDevice::init(BLE_DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    NimBLEServer *server = NimBLEDevice::createServer();
    server->setCallbacks(&server_callbacks);

    NimBLEService *service = server->createService(BLE_SERVICE_UUID);

    NimBLECharacteristic *drive = service->createCharacteristic(
        BLE_DRIVE_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    drive->setCallbacks(&drive_callbacks);

    NimBLECharacteristic *stop = service->createCharacteristic(
        BLE_STOP_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    stop->setCallbacks(&stop_callbacks);

    NimBLECharacteristic *pins = service->createCharacteristic(
        BLE_PINS_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    pins->setCallbacks(&pins_callbacks);

    service->start();

    NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
    advertising->addServiceUUID(BLE_SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->start();

    Serial.printf("[ble] advertising as %s  addr %s\n", BLE_DEVICE_NAME,
                  NimBLEDevice::getAddress().toString().c_str());
}

bool ble_is_connected() { return connected; }

uint32_t ble_last_drive_ms() { return last_drive_ms; }

uint32_t ble_last_pins_ms() { return last_pins_ms; }

void ble_get_command(int8_t *left, int8_t *right) {
    *left = cmd_left;
    *right = cmd_right;
}

void ble_get_pins(uint8_t *in1, uint8_t *in2, uint8_t *in3, uint8_t *in4) {
    *in1 = cmd_pins[0];
    *in2 = cmd_pins[1];
    *in3 = cmd_pins[2];
    *in4 = cmd_pins[3];
}
