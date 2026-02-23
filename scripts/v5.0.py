import struct
import os

def calculate_6102_crc(rom_data):
    seed = 0xF8CA4DDC
    t1 = t2 = t3 = t4 = t5 = t6 = seed
    for i in range(0x400, 0x100000, 4):
        d = struct.unpack(">I", rom_data[i:i+4])[0]
        if (t6 + d) & 0xFFFFFFFF < t6:
            t4 = (t4 + 1) & 0xFFFFFFFF
        t6 = (t6 + d) & 0xFFFFFFFF
        t3 ^= d
        r1 = (d << (d & 0x1F)) | (d >> (32 - (d & 0x1F)))
        t5 = (t5 + r1) & 0xFFFFFFFF
        if t2 > d:
            t2 ^= r1 & 0xFFFFFFFF
        else:
            t2 ^= t6 ^ d
        offset = 0x40 + (i & 0x7FC)
        t1 = (t1 + (struct.unpack(">I", rom_data[offset:offset+4])[0] ^ d)) & 0xFFFFFFFF
    return (t6 ^ t4) ^ t3, (t5 ^ t2) ^ t1

def build_rom(output_name, frequency):
    if not os.path.exists("ipl3.bin"):
        print("Error: ipl3.bin not found."); return

    # Libdragon NTSC Calculations
    clockrate = 48681818
    dacrate = int(((2 * clockrate / frequency) + 1) / 2)
    bitrate = min(16, dacrate // 66)

    # 1. Create 2MB empty ROM
    rom = bytearray(2097152) 
    
    # 2. Header
    rom[0x00:0x04] = struct.pack(">I", 0x80371240)
    rom[0x08:0x0C] = struct.pack(">I", 0x80000400)
    rom[0x0C:0x10] = struct.pack(">I", 0x00001444)
    rom[0x20:0x34] = f"ONE SHOT {frequency//1000}K".ljust(20).encode('ascii')
    rom[0x3B:0x3F] = b"NRTE" # "E" for NTSC

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
        0x3C09A010, 0x35290008, 0xAD090000, # PI_DRAM_ADDR (0xA0100008)
        0x3C091001, 0x35290000, 0xAD090004, # PI_CART_ADDR (0x10010000)
        0x34091FF7, 0xAD09000C, # PI_WR_LEN (8184 bytes)

        # Wait for PI Busy bit to clear
        0x8D090010, 0x31290001, 0x1520FFFD, 0x00000000,

        # Trigger AI (Length must be multiple of 8)
        0x3C08A450,           # lui $t0, 0xA450 (AI Base)
        0x3C09A010, 0x35290008, 0xAD090000, # AI_ADDR
        0x34091FF0, 0xAD090004, # AI_LEN (8176 bytes)
        0x34090001, 0xAD090008, # AI_CONTROL = 1
        
        # HALT (Infinite loop)
        0x1000FFFF, 0x00000000 
    ]
    for i, instr in enumerate(mips):
        rom[0x1000+(i*4) : 0x1004+(i*4)] = struct.pack(">I", instr)

    # 5. Plateau Data at ROM 0x10000 (Physical 0x10010000 for PI)
    for i in range(0x10000, 0x12000, 2):
        rom[i:i+2] = struct.pack(">h", 0x7FFF)

    # 6. Checksum
    c0, c1 = calculate_6102_crc(rom)
    rom[0x10:0x18] = struct.pack(">II", c0, c1)
    
    with open(output_name, "wb") as f:
        f.write(rom)
    print(f"✓ V4 Built: {output_name}")

if __name__ == "__main__":
    build_rom("v4_32k.z64", 32000)
    build_rom("v4_48k.z64", 48000)
    # New targets for broader comparison
    build_rom("v4_441k.z64", 44100)
    build_rom("v4_22k.z64", 22050)