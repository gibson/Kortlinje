#pragma once

#include "modules/ServiceModule.h"

#include <ArduinoJson.h>
#include <atomic>
#include <string>
#include <string_view>

class TimeService final : public ServiceModule
{
public:
    static constexpr std::string_view name{"Time"};

    explicit TimeService() : ServiceModule(name) {};

    void begin();
    void handle();

    static void onReceive(JsonObjectConst payload, std::string_view source);
    static void sync();

    [[nodiscard]] static uint8_t getFormat();

private:
    static inline std::atomic<uint8_t> format{24U};

    static inline bool pendingFormat{false};
    static inline bool pendingServer{false};
    static inline bool pendingZone{false};

    static inline uint8_t pendingFormatValue{24U};

    static inline std::string posix{};
    static inline std::string server{};
    static inline std::string zone{};

    static inline std::string pendingPosixValue{};
    static inline std::string pendingServerValue{};
    static inline std::string pendingZoneValue{};

    static void apply();
    static void transmit();
};
