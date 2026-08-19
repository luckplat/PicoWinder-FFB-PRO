/*
 * A5 HID-PID compatibility shim — logical source representation.
 *
 * This is included to make the A5 changes readable. Exact reproducible binary
 * changes are in rebuild_A5_from_A4.py.
 *
 * Added HID INPUT report:
 *   Report ID 2 = PID State Report
 *
 * GET_REPORT(INPUT,2):
 *   byte0 = 0x12; // Actuators Enabled + Actuator Power
 *   byte1 = 0x00; // no effect currently reported playing
 *
 * PID Pool feature:
 *   ramPoolSize      = 0xffff; // virtual device-managed pool
 *   simultaneousMax  = 10;     // unchanged stock value
 *   flags            = 0xff;   // unchanged stock value
 *
 * BlockLoad:
 *   effectBlockIndex and status remain stock/dynamic.
 *   ramPoolAvailable = 0xffff;
 *
 * Device Reset:
 *   physical FFP reset (C5 01)
 *   native autocentre OFF (C5 06)
 *   send PID State report ID 2 through TinyUSB IN.
 *
 * A4 effect engine / FIFO / DEFINE behavior is unchanged.
 */
