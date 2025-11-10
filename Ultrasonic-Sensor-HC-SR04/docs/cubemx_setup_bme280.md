# CubeMX Configuration Steps for BME280 on Nucleo-F439ZI

1. Launch STM32CubeMX and create a new project (`File → New Project`). Use the
   **Board Selector**, search for `NUCLEO-F439ZI`, and click `Next` then
   `Finish`.
2. In the **Pinout & Configuration** tab set up the peripherals:
   - **I2C1**
     - Enable `I2C1` via `Connectivity → I2C1` and choose `I2C` mode.
     - CubeMX maps `PB8` to `I2C1_SCL` and `PB9` to `I2C1_SDA`. Keep the
       alternate function (`AF4`).
   - **USART3**
     - Enable `USART3` in `Connectivity → USART3`, select `Asynchronous` mode.
     - Pins `PB10` (`USART3_TX`) and `PB11` (`USART3_RX`) are auto-selected.
   - (Optional) Label the pins to match the schematic, e.g. `BME280_SCL` /
     `BME280_SDA` and `VCP_TX` / `VCP_RX`.
3. Configure `System Core → GPIO` to ensure both PB8 and PB9 have pull-ups
   enabled (`Pull-up`) to support the I2C bus (external pull-ups are still
   recommended on the breakout).
4. Open `Connectivity → I2C1` parameter settings and:
   - Set `I2C Clock Speed` to `400 kHz` (Fast Mode) or `100 kHz` if wiring is
     long. Use the value you plan to debug with; 100 kHz is safest.
   - Leave addressing mode as `7-bit`. Confirm the timing is valid in the
     Timing Configuration helper.
5. Under `Connectivity → USART3` set the UART parameters to:
   - `Baud Rate = 115200`
   - `Word Length = 8 Bits`, `Stop Bits = 1`, `Parity = None`
   - Enable both `TX` and `RX`.
6. Enable the NVIC interrupt line for I2C errors (`I2C1 event` and `I2C1 error`)
   if you plan to use interrupt/DMA later. For polling mode these can remain
   disabled.
7. Confirm clock configuration: the default Nucleo board settings already run
   the MCU at 180 MHz. Ensure APB1 is at 45 MHz (so that the I2C timing wizard
   uses the right base clock).
8. In `Project Manager`:
   - Name the project (e.g. `bme280_nucleo`).
   - Choose `STM32CubeIDE` (compatible with the VS Code STM32 extension) as the
     toolchain.
   - Select your preferred code generator options (e.g. generate peripheral
     initialization in separate files).
   - Set the output directory under
     `C:\Users\Yassine El Gares\home car project\Project\temperature_humidity_pressure_sensor\bme280_sensor`.
9. Click `GENERATE CODE`, then open the generated project in VS Code.


