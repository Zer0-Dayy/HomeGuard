CubeMX Configuration Steps for HC-SR04 on Nucleo-F439ZI
=======================================================

1. Launch STM32CubeMX and create a new project via `File → New Project`
   using the Board Selector. Search for and choose `NUCLEO-F439ZI`, then
   click `Next` followed by `Finish`.
2. In the **Pinout & Configuration** tab set up the pins:
   - PA5 → `GPIO_Output` (label it `HCSR04_TRIG`). Leave default push-pull
     and enable pull-down if desired.
   - PA6 → expand `TIM3` and select `TIM3_CH1` (this becomes the Echo pin).
3. Configure **GPIO** under `System Core → GPIO`:
   - For PA5 set Mode `Output Push Pull`, Pull `No pull`, Maximum Speed
     `Very High`.
   - Ensure PA6 shows as `Alternate Function Push Pull` with AF1 (TIM2).
4. Enable TIM3 input capture via `Timers → TIM3`:
   - Clock Source `Internal Clock`.
   - Prescaler: leave blank for now (CubeMX fills after clock config).
   - Counter Period: enter `4294967295` (full 32-bit range).
   - Under `Channel1` set `Input Capture` with `Direct` mapping.
   - Set `Prescaler = 1`, `Filter = 0`, `Polarity = Rising Edge`.
5. Enable interrupts: in `NVIC` enable `TIM3 global interrupt`.
6. Configure USART3 (Virtual COM port) via `Connectivity → USART3`:
   - Mode `Asynchronous` (CubeMX auto-maps PB10/PB11).
   - Parameter Settings: `Baud Rate = 115200`, `Word Length = 8 Bits`,
     `Stop Bits = 1`, `Parity = None`, `Mode = TX/RX`.
7. Clock configuration (default board settings already give 180 MHz
   HCLK). Confirm APB1 Timer clocks at 90 MHz (TIM2 runs from APB1 ×2).
8. In `PROJECT MANAGER`:
   - Give the project a name (e.g., `hcsr04_nucleo`).
   - Under `Toolchain / IDE` choose `STM32CubeIDE` (compatible with VS Code
     STM32 extension) and tick `Generate peripheral initialization as a
     pair of .c/.h files`.
   - Output location: point to
     `C:\Users\Yassine El Gares\home car project\Project\ultra sonic sensor`.
9. Click `GENERATE CODE`. Once complete, open the folder in VS Code.

