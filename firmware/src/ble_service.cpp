/**
 * @file ble_service.cpp
 * @brief Implementation of BLE GATT Server, Characteristic notifications, and power-tuned advertising
 * 
 * Project: breath-pause-monitor-esp32
 * Author: Vigneshwar S, 2nd-year ECE, SRM Institute of Science and Technology
 */

#include "ble_service.h"

BleTelemetryServer::BleTelemetryServer()
    : _pServer(nullptr),
      _pService(nullptr),
      _pTelemetryChar(nullptr),
      _pPauseAlertChar(nullptr),
      _pConfigChar(nullptr),
      _deviceConnected(false),
      _oldDeviceConnected(false) {}

void BleTelemetryServer::begin() {
    Serial.println(F("[BLE] Initializing Bluetooth Low Energy stack..."));
    BLEDevice::init(BLE_DEVICE_NAME);

    // Create the BLE Server
    _pServer = BLEDevice::createServer();
    _pServer->setCallbacks(this);

    // Create Custom GATT Service
    _pService = _pServer->createService(BLE_SERVICE_UUID);

    // 1. Primary Telemetry Characteristic (Notify)
    _pTelemetryChar = _pService->createCharacteristic(
        BLE_CHAR_TELEMETRY_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    _pTelemetryChar->addDescriptor(new BLE2902());

    // 2. High-Priority Pause Alert Characteristic (Notify)
    _pPauseAlertChar = _pService->createCharacteristic(
        BLE_CHAR_PAUSE_ALERT_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    _pPauseAlertChar->addDescriptor(new BLE2902());

    // 3. Configuration Characteristic (Read/Write)
    _pConfigChar = _pService->createCharacteristic(
        BLE_CHAR_CONFIG_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE
    );
    _pConfigChar->addDescriptor(new BLE2902());

    // Start the service
    _pService->start();

    // Configure Advertising parameters tailored for low-power operation
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    // Min/Max interval: 100ms - 150ms advertising window
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);

    BLEDevice::startAdvertising();
    Serial.println(F("[BLE] Advertising started. Ready for Web Bluetooth dashboard connection."));
}

void BleTelemetryServer::onConnect(BLEServer* pServer) {
    _deviceConnected = true;
    Serial.println(F("[BLE] Client connected (Web Bluetooth Dashboard)."));
}

void BleTelemetryServer::onDisconnect(BLEServer* pServer) {
    _deviceConnected = false;
    Serial.println(F("[BLE] Client disconnected. Restarting advertising..."));
    // Restart advertising so dashboard can reconnect seamlessly
    BLEDevice::startAdvertising();
}

void BleTelemetryServer::sendTelemetry(const TelemetryPacket &packet) {
    if (_deviceConnected && _pTelemetryChar != nullptr) {
        _pTelemetryChar->setValue((uint8_t*)&packet, sizeof(TelemetryPacket));
        _pTelemetryChar->notify();
    }
}

void BleTelemetryServer::sendPauseAlert(const PauseAlertPacket &alert) {
    if (_deviceConnected && _pPauseAlertChar != nullptr) {
        _pPauseAlertChar->setValue((uint8_t*)&alert, sizeof(PauseAlertPacket));
        _pPauseAlertChar->notify();
    }
}
