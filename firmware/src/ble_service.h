/**
 * @file ble_service.h
 * @brief Bluetooth Low Energy Custom GATT Service and Notification Dispatch
 * 
 * Project: breath-pause-monitor-esp32
 * Author: Vigneshwar S, 2nd-year ECE, SRM Institute of Science and Technology
 */

#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "config.h"

class BleTelemetryServer : public BLEServerCallbacks {
public:
    BleTelemetryServer();

    /**
     * @brief Initialize BLE Device, Services, Characteristics, and start Advertising.
     */
    void begin();

    /**
     * @brief Send 1 Hz periodic telemetry notification to connected web dashboard.
     * @param packet Populated binary telemetry packet
     */
    void sendTelemetry(const TelemetryPacket &packet);

    /**
     * @brief Send immediate high-priority breathing pause alert event.
     * @param alert Populated pause alert packet
     */
    void sendPauseAlert(const PauseAlertPacket &alert);

    bool isClientConnected() const { return _deviceConnected; }

    // BLEServerCallbacks overrides
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;

private:
    BLEServer* _pServer;
    BLEService* _pService;
    BLECharacteristic* _pTelemetryChar;
    BLECharacteristic* _pPauseAlertChar;
    BLECharacteristic* _pConfigChar;

    bool _deviceConnected;
    bool _oldDeviceConnected;
};

#endif // BLE_SERVICE_H
