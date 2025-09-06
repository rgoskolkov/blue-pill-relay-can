#include "unity.h"
#include "relay_driver.h"
#include "input_driver.h"
#include "modbus_rtu.h"

void setUp(void) {
    // Initialize the system and peripherals
    relay_driver_init();
    input_driver_init();
    modbus_rtu_init();
}

void tearDown(void) {
    // Clean up resources if needed
}

void test_relay_control(void) {
    // Test relay activation
    input_driver_set_switch_state(0, true); // Simulate switch 0 pressed
    relay_driver_update(); // Update relay states based on switch inputs
    TEST_ASSERT_TRUE(relay_driver_get_relay_state(0)); // Check if relay 0 is activated

    input_driver_set_switch_state(0, false); // Simulate switch 0 released
    relay_driver_update(); // Update relay states
    TEST_ASSERT_FALSE(relay_driver_get_relay_state(0)); // Check if relay 0 is deactivated
}

void test_modbus_control(void) {
    // Test Modbus relay control
    modbus_rtu_set_relay(1, true); // Simulate Modbus command to activate relay 1
    relay_driver_update(); // Update relay states
    TEST_ASSERT_TRUE(relay_driver_get_relay_state(1)); // Check if relay 1 is activated

    modbus_rtu_set_relay(1, false); // Simulate Modbus command to deactivate relay 1
    relay_driver_update(); // Update relay states
    TEST_ASSERT_FALSE(relay_driver_get_relay_state(1)); // Check if relay 1 is deactivated
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_relay_control);
    RUN_TEST(test_modbus_control);
    return UNITY_END();
}