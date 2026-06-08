#include <ETH.h>
#include <ModbusRTU.h>
#include <ModbusTCP.h>

// Pins für WT32-ETH01 (Diese bleiben gleich)
#define ETH_ADDR          1
#define ETH_POWER_PIN     16
#define ETH_MDC_PIN       23
#define ETH_MDIO_PIN      18

// RS485 Pins
#define RX_PIN 5
#define TX_PIN 17

// Timing
unsigned long lastPlotTime = 0;
const unsigned long plotInterval = 3000; // 3 Sekunden

const int REG_POWER_TOTAL = 19026; 
ModbusRTU mbRTU; 
ModbusTCP mbTCP; 
float lastValue = 0.0;

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- WT32-ETH01 v3.0 Setup ---");

  // Wir nutzen direkt die vordefinierten Typen der Library
  // ETH_PHY_LAN8720 und ETH_CLOCK_GPIO0_IN sind interne Konstanten
  if (!ETH.begin(ETH_PHY_LAN8720, ETH_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLOCK_GPIO0_IN)) {
    Serial.println("Ethernet Start fehlgeschlagen!");
  }
  
  Serial1.begin(9600, SERIAL_8E1, RX_PIN, TX_PIN);
  mbRTU.begin(&Serial1);
  mbRTU.slave(1);
  mbRTU.addHreg(REG_POWER_TOTAL, 0, 2);

  mbTCP.server(); 
  mbTCP.addHreg(REG_POWER_TOTAL, 0, 2);
}

void loop() {
  static bool hasIP = false;
  
  if (ETH.linkUp()) {
    IPAddress ip = ETH.localIP();
    if (ip[0] != 0 && !hasIP) {
      Serial.print("Verbunden! IP-Adresse: ");
      Serial.println(ip);
      delay(2000);
      hasIP = true;
    }
  } else {
    if (hasIP) {
      Serial.println("Kabel entfernt oder Verbindung weg.");
      hasIP = false;
    }
  }

mbTCP.task();
mbRTU.task();

  // Alle 3 Sekunden den Wert plotten
  if (millis() - lastPlotTime >= plotInterval) {
    lastPlotTime = millis();

    uint16_t h = mbTCP.Hreg(REG_POWER_TOTAL);
    uint16_t l = mbTCP.Hreg(REG_POWER_TOTAL + 1);

    // Rohwerte anzeigen
    Serial.print("Raw Register H (19026): "); Serial.println(h);
    Serial.print("Raw Register L (19027): "); Serial.println(l);

    float powerValue;
    // Wir probieren hier die "Swapped" Version direkt aus
    uint32_t combined = ((uint32_t)h << 16) | l;
    memcpy(&powerValue, &combined, sizeof(float));

    Serial.print("Interpretierter Float: ");
    Serial.println(powerValue, 2); // 2 Nachkommastellen
  }

  // Hier evtl. noch deine Spiegelung TCP -> RTU falls nicht schon drin
  mbRTU.Hreg(REG_POWER_TOTAL, mbTCP.Hreg(REG_POWER_TOTAL));
  mbRTU.Hreg(REG_POWER_TOTAL + 1, mbTCP.Hreg(REG_POWER_TOTAL + 1));
}