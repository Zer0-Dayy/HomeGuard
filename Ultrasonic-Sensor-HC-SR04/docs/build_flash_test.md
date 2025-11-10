Build, Flash, and Test Checklist
================================

1. Open the generated project in VS Code with the STM32 extension.
2. Run `STM32: Build` from the command palette (Ctrl+Shift+P) to compile
   the CubeMX project. Resolve any HAL include path issues if prompted.
3. Connect the Nucleo board over USB. Ensure the ST-LINK drivers are
   installed.
4. Use `STM32: Flash` to program the device. Alternatively run the
   generated `make flash` target from the integrated terminal.
5. Launch a serial monitor at 115200 baud on `COMx` (ST-LINK Virtual
   COM port). The STM32 VS Code extension provides a monitor, or use
   PuTTY/Tera Term.
6. Move an object in front of the HC-SR04. You should see distance
   updates like `Distance: 23.45 cm` every 100 ms. A timeout message
   indicates no echo was captured.
7. If readings are unstable, check wiring (PA5 → Trig, PA6 → Echo, GND,
   Vcc 5 V) and verify the sensor ground is common with the board.

---

BME280 Project Notes
--------------------

1. Generate the BME280 CubeMX project under
   `temperature_humidity_pressure_sensor/bme280_sensor` with I2C1 and
   USART3 enabled per `cubemx_setup_bme280.md`.
2. After generation open the project in VS Code. Confirm `bme280.c/.h`
   and the example `main.c` are added to your build (e.g. via the STM32
   project view or by updating the `Src`/`Inc` file lists if needed).
3. Build and flash as usual (`STM32: Build`, then `STM32: Flash`). Keep
   the sensor powered at 3.3 V and share ground with the Nucleo. Connect
   PB8 → SCL and PB9 → SDA with pull-up resistors (many breakout boards
   include them).
4. Open a serial monitor at 115200 baud. Expected log lines resemble
   `Temp: 24.12 C, Humidity: 45.87 %RH, Pressure: 1008.65 hPa` once per
   second.
5. If readings fail, verify the sensor address (0x76 default). If your
   board uses 0x77, change `BME280_I2C_ADDR_DEFAULT` in `bme280.h` to
   `(0x77U << 1)`. Double-check I2C wiring and that SDA/SCL are not
   swapped.

