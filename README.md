
# ESP-IDF LVGL v9 Port for Sunton 7" Smart Display (ESP32-8048S070)

An ultra-fast, native **ESP-IDF v5.5+** port of the **LVGL v9.3.0** graphics library for the popular, budget-friendly 7-inch Sunton Smart Display (**ESP32-8048S070**). 

This project runs entirely on raw `esp_lcd` drivers with an aggressive **20 MHz Pixel Clock**, pushing hardware capabilities way beyond standard Arduino or PlatformIO abstraction layers.
![Screenshot](/PICTURES/01.jpg)
![Screenshot](/PICTURES/02.jpg)
---

## 🚀 Key Features & Performance

*   **Native Framework:** Built entirely with ESP-IDF (v5.5.3 tested). No Arduino overhead.
*   **Hardware Acceleration:** Leverages full Octal PSRAM capabilities via automated GDMA double-buffering using `LV_DISPLAY_RENDER_MODE_DIRECT`.
*   **Insane Pixel Clock:** Rock-stable display output at **20 MHz** (compared to the typical ~12.5 MHz found in LovyanGFX setups).
*   **Accurate Touch Response:** Full native support for the onboard capacitive Goodix GT911 touch controller.

---

## 🚨 The Hardware Catch: Sunton vs. VIEWE

This project was originally forked from the VIEWE 7" repository. However, empirical testing revealed that although both boards share identical core silicon (ESP32-S3), **their physical PCB routing is completely different.**

If you flash a stock VIEWE binary onto a Sunton board, you will get a severe visual distortion resembling an LCD "implosion" due to phase misalignment, followed by inverted colors.

### Hardware Mapping Differences Solved in this Repo:

1.  **Inverted Sync Lines:** 
    *   VIEWE maps `VSYNC` to GPIO 41 and `DE` to GPIO 40.
    *   **Sunton** physically flips these lines (`VSYNC` = GPIO 40, `DE` = GPIO 41).
2.  **Scrambled RGB565 Data Bus:** 
    VIEWE uses an unusual interleaved bit sequence. This repository maps the macros precisely to Sunton's hardware layout to ensure flawless grayscale gradients and accurate color reproduction:
    *   **Blue Channel (B0 - B4):** GPIOs `15, 7, 6, 5, 4`
    *   **Green Channel (G0 - G5):** GPIOs `9, 46, 3, 8, 16, 1`
    *   **Red Channel (R0 - R4):** GPIOs `14, 21, 47, 48, 45`

---

## 📊 Benchmark Results (Sunton 7" 800x480)

Here is a direct software performance comparison captured on the exact same Sunton hardware, showing the massive performance uplift of pure **ESP-IDF** over **PlatformIO + LovyanGFX**:


| Test Name | VIEWE IDF 9.3.0 <br> CPU% / FPS / ms | SUNTON IDF 9.3.0 <br> CPU% / FPS / ms | SUNTON IDF 9.5.0 <br> CPU% / FPS / ms | LOVYAN SUNTON 9.5.0 <br> CPU% / FPS / ms |
| :--- | :---: | :---: | :---: | :---: |
| **ALL SCENES AVG**    | 86% / **15** / 57(41+16)  | 84% / **15** / 55(38+16)  | 86% / **14** / 59(42+17)  | 83% / **12** / 89(62+27)   |
| **EMPTY SCREEN**      | 90% / **16** / 53(28+25)  | 90% / **17** / 50(28+22)  | 90% / **16** / 53(28+25)  | 59% / **12** / 81(32+49)   |
| **MOVING WALLPAPER**  | 96% / **12** / 73(64+9)   | 97% / **12** / 71(63+8)   | 96% / **12** / 75(64+11)  | 95% / **5**  / 158(103+55) |
| **SINGLE RECTANGLE**  | 55% / **24** / 18(4+14)   | 62% / **24** / 20(4+16)   | 56% / **24** / 18(4+14)   | 38% / **28** / 9(4+5)      |
| **MULTIPLE RECTANGLE**| 72% / **22** / 28(14+14)  | 70% / **22** / 29(15+14)  | 74% / **21** / 30(15+15)  | 75% / **17** / 50(19+31)   |
| **MULTIPLE RGB IMG**  | 71% / **23** / 22(10+12)  | 69% / **23** / 22(10+12)  | 74% / **22** / 24(11+13)  | 87% / **19** / 41(26+15)   |
| **MULTIPLE ARGB IMG** | 88% / **15** / 52(26+26)  | 88% / **15** / 52(26+26)  | 87% / **14** / 53(27+26)  | 91% / **14** / 58(42+16)   |
| **ROTATED ARGB IMG**  | 93% / **13** / 64(46+18)  | 93% / **13** / 64(46+18)  | 94% / **13** / 64(48+16)  | 93% / **12** / 73(62+11)   |
| **MULTIPLE LABELS**   | 85% / **19** / 43(24+19)  | 83% / **19** / 44(24+20)  | 88% / **16** / 48(25+23)  | 82% / **14** / 61(48+13)   |
| **SCREEN SIZED TEXT** | 94% / **9**  / 98(83+15)  | 96% / **9**  / 99(84+15)  | 96% / **9**  / 98(82+16)  | 94% / **5**  / 155(100+55) |
| **MULTIPLE ARCS**     | 67% / **23** / 20(4+16)   | 68% / **23** / 21(5+16)   | 67% / **23** / 20(5+15)   | 62% / **27** / 16(15+1)    |
| **CONTAINERS**        | 93% / **14** / 57(34+23)  | 92% / **14** / 58(34+24)  | 92% / **14** / 58(35+23)  | 89% / **10** / 81(61+20)   |
| **CONTAINERS W. OVL** | 95% / **10** / 76(63+13)  | 94% / **10** / 76(61+15)  | 96% / **10** / 82(67+15)  | 95% / **7**  / 115(89+26)  |
| **CONTAINERS W. OPA** | 94% / **13** / 60(43+17)  | 94% / **13** / 59(41+18)  | 94% / **13** / 60(44+16)  | 95% / **8**  / 102(80+22)  |
| **CONTAINERS W. LYR** | 96% / **8**  / 104(95+9)  | 96% / **8**  / 101(93+8)  | 96% / **7**  / 115(95+20) | 96% / **6**  / 148(125+23) |
| **CONTAINERS W. SCR** | 93% / **13** / 65(53+12)  | 95% / **13** / 65(52+13)  | 95% / **13** / 65(54+11)  | 95% / **7**  / 133(78+55)  |
| **WIDGETS DEMO**      | 96% / **8**  / 87(68+19)  | 96% / **8**  / 87(68+19)  | 96% / **8**  / 83(69+14)  | 96% / **5**  / 151(108+43) |

*Note: Time values are represented as Total Time (Render Time + Flush Time) in milliseconds.*


---

## 🛠️ Getting Started

### Prerequisites
*   Visual Studio Code with official **ESP-IDF Extension**.
*   **ESP-IDF v5.3+** installed (v5.5.3 recommended).

### How to Flash
1.  Clone this repository.
2.  Open the project directory in VS Code.
3.  Clean up any temporary workspace states and set the target:
    ```bash
    idf.py fullclean
    idf.py set-target esp32s3
    ```
4.  Build, Flash, and Monitor:
    ```bash
    idf.py build flash monitor
    ```

---

 
### Benchmark Results (800x480)

