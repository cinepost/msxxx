# MSX2+ FPGA Implementation

An advanced FPGA-based recreation of the MSX2+ architecture, optimized for high-fidelity video and audio output. This project leverages the Altera Cyclone I (EP1C12) and an external MCU for system management.

## ⚙️ System Architecture
- **Core Logic**: Implemented on an **Altera Cyclone I (12K LE)**, capturing the full MSX2+ specification.
- **System Controller**: An **RPi Pico (RP2040)** handles initial FPGA configuration (bitstream loading) and real-time system settings.
- **Video Path**: Features a dedicated **ADV7123 Triple DAC** for high-precision signal generation, bypassing standard FPGA-pin PWM limitations.
- **Audio Path**: **Sigma-Delta audio output** for superior sound clarity and reduced noise floor.

## 📺 Video Features
Supporting a wide array of legacy and modern connections via dedicated driver chips:
- **RGB / VGA**: High-quality analog output via the ADV7123.
- **S-Video & CVBS**: Dedicated encoding hardware for authentic retro display compatibility.
- **Improved Signal Integrity**: Specialized circuitry to ensure stable sync and color reproduction across all outputs.

## 🔊 High-Fidelity Audio
This implementation moves beyond standard FPGA PWM audio, featuring a dedicated high-end analog stage:
- **Primary DAC**: **Cirrus Logic CS4398** (120dB Dynamic Range, 24-bit/192kHz capable) for superior Delta-Sigma D/A conversion.
- **Output Stage**: **NE5532 Op-Amp** configured for low-noise buffering and active filtering.
- **Audio Quality**: Crystal-clear MSX sound synthesis with significantly reduced harmonic distortion and a professional-grade noise floor.

## 🛠️ Hardware Requirements
*   FPGA: Altera Cyclone I (EP1C12 version).
*   MCU: Raspberry Pi Pico or compatible RP2040 board.
*   Video DAC: Analog Devices ADV7123.
*   Supporting discrete components for video filtering and audio amplification.

## 🚀 Getting Started
1. **Flash the MCU**: Load the RP2040 firmware to manage the FPGA bridge.
2. **FPGA Bitstream**: Place the compiled `.rbf` or `.bin` file on the designated storage (SD card/Internal Flash).
3. **Power On**: The Pico will configure the Cyclone I and initialize the MSX environment.

## 🔧 Future Improvements
- [ ] Expanded memory mapper support.
- [ ] Additional sound chip synthesis (SCC+, OPLL).
- [ ] Enhanced firmware interface for the RP2040 OSD.

## 📄 License
This project is licensed under the [MIT License](LICENSE).
