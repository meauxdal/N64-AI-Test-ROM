N64 Audio Interface Test (AI) ROM for hardware probing and emulator improvement. Performs DMA transfers to characterize analog output and DAC behavior. Largely libdragon-based.

**Note: Turn your volume down.** 

Plays back PCM data with a constant amplitude of 0x7FFF for primary tests. The standard sweep lasts 2048 samples per frequency, while the legacy sweep retains 4088 samples to match older v5 test ROMs. Each test is separated by a 1 second wait period to allow output capacitors to fully discharge on hardware.

| Sample Rate Target | AI_DACRATE | AI_BITRATE | Standard (2048 Samples) | Legacy V5 Sweep (4088 Samples) |
| --- | --- | --- | --- | --- |
| 22050 Hz	| 2208 | 16 | 92.88 ms	| 185.40 ms | 
| 32000 Hz	| 1521 | 16 | 64.00 ms	| 127.75 ms |
| 44100 Hz	| 1104 | 16 | 46.44 ms	| 92.70 ms | 
| 48000 Hz	| 1014 | 15 | 42.67 ms	| 85.17 ms |

**AI_BITRATE = AI_DACRATE / 66 (minimum 16) REG_AI_BITRATE = AI_BITRATE - 1**

Because the AI uses a clock divider, sample rate targets are approximate. The actual values written to the AI_DACRATE and AI_BITRATE registers equal the corresponding values in the above table, minus 1.

Edge case behavior is WIP. Edge Cases: Includes maximum negative DC (0x8001), Nyquist torture tests (alternating 0x7FFF/0x8001 every sample), and low-frequency clock stress.

Evolved from earlier ROM generator scripts, currently available [here](https://www.mediafire.com/folder/wyso4tkwci9ln/N64-audio) along with samples, simpler test ROMs, and comparison images
