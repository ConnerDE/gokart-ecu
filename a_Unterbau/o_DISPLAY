
/* ==================== DISPLAY ==================== */
void updateDisplay(int8_t gear, SafetyModule& safety, CanReceiver& can) {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 100) return; // 10 FPS
  lastUpdate = millis();

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (safety.isEmergencyActive()) {
    display.setTextSize(2);
    display.setCursor(10, 10);
    display.print("ERROR:");
    display.setTextSize(3);
    display.setCursor(10, 35);
    if (!safety.oilPresent) display.print("E1");
    else if (safety.tiltDetected) display.print("E2");
    else if (safety.oilTemp > OIL_TEMP_CRITICAL) display.print("E3");
    else if (safety.batteryVoltage < BATTERY_CUTOFF) display.print("E4");
  }
  else if (!can.dataValid) {
    display.setTextSize(2);
    display.setCursor(10, 25);
    display.print("NO CAN");
  }
  else {
    // Gear (Big)
    display.setTextSize(4);
    display.setCursor(5, 15);
    if (gear == 0) display.print("N");
    else if (gear == -1) display.print("R");
    else display.print(gear);

    // RPM Bar (Bottom)
    int barWidth = map(safety.currentRPM, 0, 4000, 0, 128);
    display.fillRect(0, 56, barWidth, 8, SSD1306_WHITE);

    // Info (Right Side)
    display.setTextSize(1);
    display.setCursor(60, 5);
    display.printf("BAT:%.1fV", safety.batteryVoltage);
    display.setCursor(60, 15);
    display.printf("OIL:%.0fC", safety.oilTemp);
    display.setCursor(60, 25);
    display.printf("SPD:%.0f", currentSpeed);

    // Mode
    display.setCursor(60, 35);
    switch(can.driveMode) {
      case 0: display.print("NRML"); break;
      case 1: display.print("SPRT"); break;
      case 2: display.print("S+"); break;
      case 3: display.print("RACE"); break;
    }
  }
  display.display();
}
