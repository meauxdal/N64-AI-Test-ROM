import struct
import os
import subprocess

def build_rom(target_frequency):
    if not os.path.exists("ipl3.bin"):
        print("Error: ipl3.bin not found."); return

    # Libdragon NTSC Calculations
    clockrate = 48681818
    dacrate = int(((2 * clockrate / target_frequency) + 1) / 2)
    bitrate = min(16, dacrate // 66)
    
    # 1. Create 2MB empty ROM
    rom = bytearray(2097152) 
    
    # 2. Header
    rom[0x00:0x04] = struct.pack(">I", 0x80371240)
    rom[0x08:0x0C] = struct.pack(">I", 0x80000400)
    rom[0x0C:0x10] = struct.pack(">I", 0x00001444)
    rom[0x20:0x34] = f"AI BUFFER TEST".ljust(20).encode('ascii')
    rom[0x3B:0x3F] = b"NRTE" 

    # 3. IPL3
    with open("ipl3.bin", "rb") as f:
        rom[0x40:0x40+4032] = f.read()[:4032]

    # 4. MIPS Logic (One-Shot Trigger)
    mips = [
        # Setup AI DAC Rates
        0x3C08A450,           # lui $t0, 0xA450 (AI Base)
        0x34090000 | (dacrate - 1), 0xAD090010, # AI_DACRATE
        0x34090000 | (bitrate - 1), 0xAD090014, # AI_BITRATE
        
        # PI DMA: Copy 8KB from Cart 0x10010000 to RAM 0xA0100008
        0x3C08A460,           # lui $t0, 0xA460 (PI Base)
        0x3C09A010, 0x35290008, 0xAD090000, # PI_DRAM_ADDR
        0x3C091001, 0x35290000, 0xAD090004, # PI_CART_ADDR
        0x34091FF7, 0xAD09000C, # PI_WR_LEN (8184 bytes)

        # Wait for PI Busy bit to clear
        0x8D090010, 0x31290001, 0x1520FFFD, 0x00000000,

        # Trigger AI
        0x3C08A450,           # lui $t0, 0xA450 (AI Base)
        0x3C09A010, 0x35290008, 0xAD090000, # AI_ADDR
        0x34091FF0, 0xAD090004, # AI_LEN (8176 bytes)
        0x34090001, 0xAD090008, # AI_CONTROL = 1
        
        # HALT (Infinite loop)
        0x1000FFFF, 0x00000000 
    ]
    for i, instr in enumerate(mips):
        rom[0x1000+(i*4) : 0x1004+(i*4)] = struct.pack(">I", instr)

    # 5. Payload: 16 samples held at 0x3FFF
    sample_count = 16
    amplitude = 0x3FFF
    for i in range(sample_count):
        rom[0x10000 + (i*2) : 0x10002 + (i*2)] = struct.pack(">h", amplitude)

    # 6. Filename Format: v6_Amp3FFF_16S_DR[DACRATE]_[Target]KHz.z64
    freq_label = f"{target_frequency/1000:g}KHz" 
    output_name = f"v6_Amp{amplitude:04X}_{sample_count}S_DR{dacrate}_{freq_label}.z64"

    with open(output_name, "wb") as f:
        f.write(rom)
    print(f"Generated: {output_name}")

if __name__ == "__main__":
    # Test frequencies: 22.05, 32, 44.1, 48
    freqs = [22050, 32000, 44100, 48000]

    for f in freqs:
        build_rom(f)

    # 7. Final directory update for headers
    print("\nRunning rn64crc.exe -u...")
    try:
        subprocess.run(["rn64crc.exe", "-u"], check=True)
        print("Success.")
    except FileNotFoundError:
        print("Error: rn64crc.exe not found in directory.")