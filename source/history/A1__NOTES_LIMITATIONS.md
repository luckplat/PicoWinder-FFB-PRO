# Known limitations of BASE A1

- The stock PicoWinder FEATURE/CreateNewEffect path is intentionally retained to avoid the TEST18/19 CreateEffect regression.
- Periodic phase is not yet translated in A1; magnitude, offset, frequency and sample-rate are translated.
- Trigger/envelope semantics are not yet complete for every HID-PID corner case.
- The output engine suppresses duplicate MIDI updates and sheds streaming parameter updates if the 4096-byte MIDI FIFO is above 75% occupancy; control/operation commands are still queued.
- A1 is designed first to recover the reliable stock input path and prevent Condor FFB traffic from blocking it. Full Condor effect fidelity must be validated on the real joystick.
