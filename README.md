

N64 Audio Interface Test (AI) ROM for hardware probing and emulator improvement. Performs DMA transfers to characterize analog output and DAC behavior. libdragon-based.

**Note: turn your volume down.** 

<img width="1282" height="1036" alt="image" src="https://github.com/user-attachments/assets/083b5ff7-1be2-414d-8889-82622e0c3f75"/>

There are three tests. The first and third tests playback PCM data with a constant amplitude of 0x7FFF. The standard test lasts 2048 samples per sample rate, while the legacy sweep retains 4088 samples to match older v5 test ROMs. Each test is separated by a 1 second wait period to allow output capacitors to fully discharge on hardware. The length and relevant AI values are indicated in the table below.

| Sample Rate Target | AI_DACRATE | AI_BITRATE | Standard (2048 Samples) | Legacy V5 Sweep (4088 Samples) |
| --- | --- | --- | --- | --- |
| 22050 Hz	| 2208 | 16 | 92.88 ms	| 185.40 ms | 
| 32000 Hz	| 1521 | 16 | 64.00 ms	| 127.75 ms |
| 44100 Hz	| 1104 | 16 | 46.44 ms	| 92.70 ms | 
| 48000 Hz	| 1014 | 15 | 42.67 ms	| 85.17 ms |

Formulae:

**AI_DACRATE = ((2 * 48681818 / Sample Rate Target) + 1) / 2**

**AI_BITRATE = AI_DACRATE / 66 (the value of AI_BITRATE cannot exceed 16)**

Note the actual values written to the AI_DACRATE and AI_BITRATE registers equal the corresponding values in the above table, minus 1.

**REG_AI_DACRATE = AI_DACRATE - 1**

**REG_AI_BITRATE = AI_BITRATE - 1**

Because the AI uses a clock divider, actual sample rate output is an approximation derived from the system clock and will vary slightly from the target frequency depending on how closely the integer division aligns. 

The second test may be expanded for additional edge case testing. Currently tests max negative DC (0x8001), Nyquist torture test (alternating 0x7FFF/0x8001 every sample), and a low-frequency clock stress. 

Evolved from earlier ROM generator scripts, currently available [here](https://www.mediafire.com/folder/wyso4tkwci9ln/N64-audio) along with samples, simpler test ROMs, and comparison images
