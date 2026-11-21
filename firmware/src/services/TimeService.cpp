#include "services/TimeService.h"

#include "constants/secrets.h"
#include "extensions/AlarmExtension.h"
#include "handlers/ConfigHandler.h"
#include "services/DeviceService.h"

#include <WiFi.h>
#include <hardware/watchdog.h>
#include <time.h>

/**
 * @brief Loads the saved time zone, NTP server and clock format, falling back to the build defaults.
 */
void TimeService::begin()
{
    if (!ConfigHandler::get(name, "zone", zone) || !ConfigHandler::get(name, "posix", posix) || posix.empty())
    {
        zone = Kortlinje::time::zone;
        posix = Kortlinje::time::posix;
    }
    if (!ConfigHandler::get(name, "server", server))
    {
        server.clear();
    }
    uint8_t _format{static_cast<uint8_t>(Kortlinje::time::format)};
    if (!ConfigHandler::get(name, "format", _format) || (_format != 12U && _format != 24U))
    {
        _format = static_cast<uint8_t>(Kortlinje::time::format);
    }
    format = _format;
    apply();
    transmit();
}

void TimeService::handle()
{
    if (!pendingFormat && !pendingServer && !pendingZone)
    {
        return;
    }
    if (pendingZone)
    {
        pendingZone = false;
        zone = std::move(pendingZoneValue);
        posix = std::move(pendingPosixValue);
        pendingZoneValue.clear();
        pendingPosixValue.clear();
        ConfigHandler::set(name, "zone", zone);
        ConfigHandler::set(name, "posix", posix);
        apply();
        AlarmExtension::reschedule();
    }
    if (pendingFormat)
    {
        pendingFormat = false;
        format = pendingFormatValue;
        ConfigHandler::set(name, "format", pendingFormatValue);
    }
    if (pendingServer)
    {
        pendingServer = false;
        server = std::move(pendingServerValue);
        pendingServerValue.clear();
        ConfigHandler::set(name, "server", server);
        if (WiFi.connected())
        {
            sync();
        }
    }
    transmit();
}

/**
 * @brief Applies the POSIX time zone rule used for local time and daylight saving time transitions.
 */
void TimeService::apply()
{
    setenv("TZ", posix.c_str(), 1);
    tzset();
}

uint8_t TimeService::getFormat() { return format; }

/**
 * @brief Handles incoming time zone, NTP server and clock format settings.
 *
 * @param payload Incoming payload containing any of zone and posix, server, and format.
 * @param source Source identifier for the payload.
 */
void TimeService::onReceive(JsonObjectConst payload, std::string_view source)
{
    if (payload["zone"].is<std::string_view>() && payload["posix"].is<std::string_view>() &&
        !payload["posix"].as<std::string_view>().empty())
    {
        pendingZoneValue = payload["zone"].as<std::string_view>();
        pendingPosixValue = payload["posix"].as<std::string_view>();
        pendingZone = true;
    }
    if (payload["server"].is<std::string_view>())
    {
        pendingServerValue = payload["server"].as<std::string_view>();
        pendingServer = true;
    }
    if (payload["format"].is<uint8_t>() &&
        (payload["format"].as<uint8_t>() == 12U || payload["format"].as<uint8_t>() == 24U))
    {
        pendingFormatValue = payload["format"].as<uint8_t>();
        pendingFormat = true;
    }
}

/**
 * @brief Starts NTP synchronization against the configured server, with the NTP pool as fallback.
 */
void TimeService::sync()
{
    if (server.empty())
    {
        NTP.begin("1.pool.ntp.org", "2.pool.ntp.org");
    }
    else
    {
        NTP.begin(server.c_str(), "pool.ntp.org");
    }
    watchdog_update();
    NTP.waitSet(0b1U << 14U);
    watchdog_update();
}

/**
 * @brief Transmits the current time zone, NTP server and clock format.
 */
void TimeService::transmit()
{
    JsonDocument doc{};
    doc["format"].set(format.load());
    doc["posix"].set(posix);
    doc["server"].set(server);
    doc["zone"].set(zone);
    DeviceService::transmit(doc.as<JsonObjectConst>(), name);
}
