#include <cassert>
#include <cstring>
#include <iostream>
#include "Controller/Core0/SystemController.h"
#include "Controller/Core1/SettingsFlash.h"
#include "Controller/Core1/SettingsManager.h"
#include "Controller/Core1/Automations.h"
#include "utils/Core1RecoveryTimer.h"

static size_t jedec_transfer_length = 0;

int spi_write_read_blocking(spi_inst_t *, const uint8_t *tx, uint8_t *rx, size_t len) {
    const uint8_t command = tx[0];
    if (command == 0x9f) {
        jedec_transfer_length = len;
        const uint8_t response[] = {0, 0xef, 0x40, 0x18};
        // Write the requested length, so AddressSanitizer also catches the old
        // five-byte transfer into a four-byte buffer in the real driver.
        for (size_t i = 0; i < len; ++i) rx[i] = i < sizeof(response) ? response[i] : 0;
    } else {
        assert(command == 0x05); // Status register: not busy.
        std::memset(rx, 0, len);
    }
    return len;
}
int spi_write_blocking(spi_inst_t *, const uint8_t *, size_t len) { return len; }
int spi_read_blocking(spi_inst_t *, uint8_t, uint8_t *rx, size_t len) {
    std::memset(rx, 0, len);
    return len;
}

struct ControllerFixture {
    PicoQueue<SystemControllerStatusMessage> status{100};
    PicoQueue<SystemControllerCommand> commands{100};
    uart_inst_t uart;
    SystemController controller{&uart, &status, &commands};
    ControllerFixture() {
        test_now = 1000000;
        controller.internalState = RUNNING;
        controller.currentControlBoardParsedPacket = {};
        controller.currentControlBoardParsedPacket.brew_boiler_temperature = 25;
        controller.currentControlBoardParsedPacket.service_boiler_temperature = 25;
    }
    ~ControllerFixture() { delete controller.settings; }
    void sleepCommand(bool enabled) {
        SystemControllerCommand command{};
        command.type = COMMAND_SET_SLEEP_MODE;
        command.bool1 = enabled;
        commands.addBlocking(&command);
        controller.handleCommands();
    }
};

void testFlashId() {
    spi_inst_t spi;
    SettingsFlash flash(&spi, 17);
    assert(flash.get_device_id() == 0x4018);
    assert(jedec_transfer_length == 4);
}

void testCore1Recovery() {
    Core1RecoveryTimer timer;
    assert(!timer.shouldRestart(false, 0));
    assert(!timer.shouldRestart(true, 1000000));
    assert(!timer.shouldRestart(true, 2999999));
    // Recovery exactly when the timeout expires takes priority over restart.
    assert(!timer.shouldRestart(false, 3000000));
    assert(!timer.shouldRestart(false, 20000000));
    // A later stall receives a fresh two-second grace period.
    assert(!timer.shouldRestart(true, 21000000));
    assert(timer.shouldRestart(true, 23000000));
    assert(!timer.shouldRestart(true, 27999999));
    assert(timer.shouldRestart(true, 28000000));
    // Successful restart must cancel the five-second retry permanently.
    assert(!timer.shouldRestart(false, 28100000));
    assert(!timer.shouldRestart(false, 40000000));
    assert(!timer.shouldRestart(true, 41000000));
    assert(timer.shouldRestart(true, 43000000));
}

void testHeatupRejectsSleep() {
    ControllerFixture f;
    auto &c = f.controller;
    f.sleepCommand(true); // No temperature report yet: not eligible for sleep.
    assert(!c.settings->getSleepMode());
    c.handleRunningStateAutomations();
    assert(c.runState == RUN_STATE_HEATUP_STAGE_1);
    f.sleepCommand(true);
    assert(!c.settings->getSleepMode());
    assert(c.runState == RUN_STATE_HEATUP_STAGE_1);
    c.currentControlBoardParsedPacket.brew_boiler_temperature = 129;
    c.handleRunningStateAutomations();
    assert(c.runState == RUN_STATE_HEATUP_STAGE_2);
    const auto started = c.heatupStage2Timer.value();
    f.sleepCommand(true);
    assert(!c.settings->getSleepMode());
    assert(c.heatupStage2Timer.value() == started);
    test_now = started + 240000000;
    c.handleRunningStateAutomations();
    assert(c.runState == RUN_STATE_HEATUP_STAGE_2);
    ++test_now;
    c.handleRunningStateAutomations();
    assert(c.runState == RUN_STATE_NORMAL);
    assert(!c.heatupStage2Timer.has_value());
    f.sleepCommand(true);
    assert(c.settings->getSleepMode());
}

void testWakeReadiness() {
    ControllerFixture f;
    auto &c = f.controller;
    c.runState = RUN_STATE_NORMAL;
    c.operationalReady = true;
    c.inBandSince = 1;
    f.sleepCommand(true);
    assert(c.settings->getSleepMode());
    assert(!c.operationalReady);
    assert(!c.inBandSince.has_value());
    test_now += 60000000;
    c.handleRunningStateAutomations();
    assert(c.runState == RUN_STATE_NORMAL);
    f.sleepCommand(false);
    // Even after cooling below the cold-start threshold, an ordinary wake
    // must not restart the 130 C accelerated heat-up sequence.
    c.currentControlBoardParsedPacket.brew_boiler_temperature = 25;
    c.handleRunningStateAutomations();
    assert(c.runState == RUN_STATE_NORMAL);
    assert(!c.heatupStage2Timer.has_value());
    assert(!c.operationalReady);
    c.currentControlBoardParsedPacket.brew_boiler_temperature = c.settings->getTargetBrewTemp();
    c.currentControlBoardParsedPacket.service_boiler_temperature = c.settings->getTargetServiceTemp();
    c.handleRunningStateAutomations();
    test_now += 29999999;
    c.handleRunningStateAutomations();
    assert(!c.operationalReady);
    ++test_now;
    c.handleRunningStateAutomations();
    assert(c.operationalReady);
}

void testRestoredSleep() {
    ControllerFixture f;
    auto &c = f.controller;
    c.internalState = NOT_STARTED_YET;
    f.sleepCommand(true); // Preserve an asleep state restored after watchdog reboot.
    c.internalState = RUNNING;
    c.handleRunningStateAutomations();
    assert(c.settings->getSleepMode());
    assert(c.runState == RUN_STATE_UNDETEMINED);
    assert(!c.heatupStage2Timer.has_value());
    f.sleepCommand(false);
    c.currentControlBoardParsedPacket.brew_boiler_temperature = 70;
    c.handleRunningStateAutomations();
    assert(c.runState == RUN_STATE_NORMAL);
}

void testSleepSettingsAndAutoSleep() {
    PicoQueue<SystemControllerCommand> commands(100);
    SettingsManager settings(&commands, nullptr);
    settings.setSleepMode(true);
    assert(commands.isEmpty());
    assert(!settings.getSleepMode());
    SystemControllerStatusMessage status{};
    status.internalState = RUNNING;
    status.runState = RUN_STATE_HEATUP_STAGE_1;
    settings.updateSleepState(status);
    settings.setSleepMode(true);
    assert(commands.isEmpty());
    assert(!settings.getSleepMode());
    status.runState = RUN_STATE_HEATUP_STAGE_2;
    settings.updateSleepState(status);
    settings.currentSettings.autoSleepMin = 1;
    test_now = 1000000;
    Automations automations(&settings, &commands);
    automations.loop(status);
    test_now += 61000000;
    for (int i = 0; i < 100; ++i) automations.loop(status);
    assert(commands.isEmpty()); // Expired auto-sleep cannot fill the command queue.
    assert(!settings.getSleepMode());
    status.runState = RUN_STATE_NORMAL;
    settings.updateSleepState(status);
    automations.loop(status);
    SystemControllerCommand command;
    assert(commands.tryRemove(&command));
    assert(command.type == COMMAND_SET_SLEEP_MODE && command.bool1);
    assert(settings.getSleepMode());
    // Core 0 remains authoritative if it rejects a request based on newer state.
    status.runState = RUN_STATE_HEATUP_STAGE_1;
    settings.updateSleepState(status);
    assert(!settings.getSleepMode());
}

void testAcceptedBrewAndLeverRearm() {
    ControllerFixture f;
    auto &c = f.controller;
    auto packet = c.currentControlBoardParsedPacket;
    auto apply = [&]() {
        c.currentControlBoardParsedPacket = packet;
        return c.handleControlBoardPacket(packet);
    };
    packet.brew_switch = true;
    assert(!apply().pump_on);
    assert(!c.isBrewActive());
    c.operationalReady = true;
    assert(!apply().pump_on); // Becoming ready while held up must not start.
    assert(!c.isBrewActive());
    packet.brew_switch = false;
    apply();
    packet.brew_switch = true;
    assert(apply().pump_on);
    assert(c.isBrewActive());
    c.flowMode = PUMP_OFF_SOLENOID_OPEN;
    assert(!apply().pump_on);
    assert(c.isBrewActive()); // Pump-off pre-infusion remains the same shot.
    packet.water_tank_empty = true;
    apply();
    test_now += 1100000;
    apply();
    c.flowMode = PUMP_ON_SOLENOID_OPEN;
    assert(apply().pump_on); // Shot saving with the tank-empty latch active.
    assert(c.isBrewActive());
    c.softBail(BAIL_REASON_CB_UNRESPONSIVE);
    assert(!c.isBrewActive());
    c.internalState = RUNNING; // Do not change the existing error recovery policy.
    packet.brew_switch = false;
    apply();
    assert(!c.isBrewActive());
    packet.brew_switch = true;
    assert(!apply().pump_on); // A new shot is blocked by the empty tank.
    assert(!c.isBrewActive());
}

int main() {
    testAcceptedBrewAndLeverRearm();
    testFlashId();
    testCore1Recovery();
    testHeatupRejectsSleep();
    testWakeReadiness();
    testRestoredSleep();
    testSleepSettingsAndAutoSleep();
    std::cout << "PASS: 7 regression scenarios (accepted brew/lever re-arm/shot saving, flash ID, Core 1 recovery, heat-up sleep gate, "
                 "wake readiness, restored sleep, manual/automatic sleep settings)\n";
}
