import struct
import os
import subprocess

def build_diag_rom():
    if not os.path.exists("ipl3.bin"):
        print("Error: ipl3.bin not found.")
        return

    target_frequency = 44100
    clockrate = 48681818
    dacrate = int(((2 * clockrate / target_frequency) + 1) / 2)
    bitrate = min(16, dacrate // 66)

    # Memory map (all uncached KSEG1 = 0xA0000000 base):
    #
    #   0xA0100008  Audio buffer 1 (2048 bytes = 0x800)
    #               Ends at 0xA0100808 -- safely away from 0x2000 boundary
    #   0xA0100808  Audio buffer 2 (2048 bytes = 0x800)
    #               Ends at 0xA0101008 -- still safe
    #   0xA0102000  Trace buffer (sentinel + 256 samples + sentinel = 1032 bytes)
    #
    # Trace layout (all 32-bit words):
    #   0xA0102000  SENTINEL_START (0xDEADBEEF)
    #   0xA0102004  Phase 1: 128 x AI_STATUS samples (512 bytes, offsets 0x004..0x200)
    #   0xA0102204  Phase 2: 128 x AI_STATUS samples (512 bytes, offsets 0x204..0x400)
    #   0xA0102404  SENTINEL_END   (0xCAFEBABE)
    #
    # To read back via SC64 after halt:
    #   sc64deployer dump 0x00102000 1032 trace.bin
    #
    # Phase 1 captures state immediately after trigger 1 (BUSY should go high,
    #   FULL should stay low).
    # Phase 2 captures state immediately after trigger 2 (FULL should go high
    #   immediately, then drop independently when buffer 1 finishes playing
    #   while BUSY stays high). This independent FULL/BUSY lifecycle is what
    #   ares does not correctly model.

    AUDIO1_PHYS  = 0x00100008
    AUDIO2_PHYS  = 0x00100808
    AUDIO_LEN    = 0x800       # 2048 bytes per transfer (must be multiple of 8)
    # PI_WR_LEN takes (bytes-1), so 0x7FF transfers 0x800 bytes

    TRACE_BASE     = 0xA0102000
    SENTINEL_START = 0xDEADBEEF
    SENTINEL_END   = 0xCAFEBABE

    rom = bytearray(2097152)

    # --- Header ---
    rom[0x00:0x04] = struct.pack(">I", 0x80371240)
    rom[0x08:0x0C] = struct.pack(">I", 0x80000400)
    rom[0x0C:0x10] = struct.pack(">I", 0x00001444)
    rom[0x20:0x34] = "N64 AI DIAG".ljust(20).encode('ascii')
    rom[0x3B:0x3F] = b"NRTE"

    # --- IPL3 ---
    with open("ipl3.bin", "rb") as f:
        rom[0x40:0x40 + 4032] = f.read()[:4032]

    # --- Audio payload at ROM 0x10000 (PI cart addr 0x10010000) ---
    # 0x7FFF DC offset across both buffers (0x1000 bytes total = 2x 0x800)
    for i in range(0x10000, 0x10000 + AUDIO_LEN * 2, 2):
        rom[i:i+2] = struct.pack(">h", 0x7FFF)

    # --- MIPS helpers ---
    def lui(rt, imm):         return 0x3C000000 | (rt   << 16) | (imm & 0xFFFF)
    def ori(rt, rs, imm):     return 0x34000000 | (rs   << 21) | (rt << 16) | (imm & 0xFFFF)
    def sw(rt, offset, base): return 0xAC000000 | (base << 21) | (rt << 16) | (offset & 0xFFFF)
    def lw(rt, offset, base): return 0x8C000000 | (base << 21) | (rt << 16) | (offset & 0xFFFF)
    def andi(rt, rs, imm):    return 0x30000000 | (rs   << 21) | (rt << 16) | (imm & 0xFFFF)
    def addiu(rt, rs, imm):   return 0x24000000 | (rs   << 21) | (rt << 16) | (imm & 0xFFFF)
    def bne(rs, rt, offset):  return 0x14000000 | (rs   << 21) | (rt << 16) | (offset & 0xFFFF)
    NOP  = 0x00000000
    HALT = 0x1000FFFF  # beq $zero, $zero, -1 (branch to self)

    T0, T1, T2, T3, T4, T5 = 8, 9, 10, 11, 12, 13
    ZERO = 0

    # Capture loop is exactly 5 instructions before the bne:
    #   lw, sw, addiu, addiu, bne+nop
    # bne offset = -5 branches back to the lw. DO NOT insert instructions
    # into the loop body without updating this offset.
    LOOP_BACK = -5

    mips = [
        # ----------------------------------------------------------------
        # Step 1: PI DMA -- copy 0x1000 bytes of audio from cart to RAM
        #   Cart 0x10010000 -> RAM 0x00100008
        #   Covers both audio buffers in one transfer
        # ----------------------------------------------------------------
        lui(T5, 0xA460),                             # $t5 = PI base (0xA4600000)
        lui(T1, 0xA010), ori(T1, T1, 0x0008),        # $t1 = 0xA0100008
        sw(T1, 0x0000, T5),                          # PI_DRAM_ADDR = 0x00100008
        lui(T1, 0x1001), ori(T1, T1, 0x0000),        # $t1 = 0x10010000
        sw(T1, 0x0004, T5),                          # PI_CART_ADDR = 0x10010000
        ori(T1, ZERO, (AUDIO_LEN * 2) - 1),          # PI_WR_LEN: (bytes-1), transfers 0x1000 bytes
        sw(T1, 0x000C, T5),                          # PI_WR_LEN

        # ----------------------------------------------------------------
        # Step 2: Wait for PI DMA to complete
        # ----------------------------------------------------------------
        lw(T1, 0x0010, T5),                          # PI_STATUS
        andi(T1, T1, 0x0001),                        # isolate PI busy bit (bit 0)
        bne(T1, ZERO, -3),                           # loop back to lw if busy
        NOP,                                         # branch delay slot

        # ----------------------------------------------------------------
        # Step 3: Configure AI clock -- RATES BEFORE TRIGGER (matches V5)
        # ----------------------------------------------------------------
        lui(T0, 0xA450),                             # $t0 = AI base (0xA4500000)
        ori(T1, ZERO, dacrate - 1),
        sw(T1, 0x0010, T0),                          # AI_DACRATE
        ori(T1, ZERO, bitrate - 1),
        sw(T1, 0x0014, T0),                          # AI_BITRATE

        # ----------------------------------------------------------------
        # Step 4: Set AI_DRAM_ADDR for buffer 1
        # ----------------------------------------------------------------
        ori(T1, ZERO, AUDIO1_PHYS & 0xFFFF),         # $t1 = 0x0008 (low 16 bits of 0x00100008)
        sw(T1, 0x0000, T0),                          # AI_DRAM_ADDR = 0x00100008
                                                     # Note: lui not needed, high bits are 0x0010
                                                     # and AI_DRAM_ADDR only holds 24 bits

        # ----------------------------------------------------------------
        # Step 5: Write sentinel and init trace pointer
        # ----------------------------------------------------------------
        lui(T2, (TRACE_BASE >> 16) & 0xFFFF),
        ori(T2, T2, TRACE_BASE & 0xFFFF),            # $t2 = 0xA0102000
        lui(T1, (SENTINEL_START >> 16) & 0xFFFF),
        ori(T1, T1, SENTINEL_START & 0xFFFF),        # $t1 = 0xDEADBEEF
        sw(T1, 0x0000, T2),                          # trace[0] = SENTINEL_START
        addiu(T2, T2, 4),                            # $t2 -> first sample slot

        # ----------------------------------------------------------------
        # Step 6: TRIGGER 1 -- enqueue buffer 1
        #   Writing AI_LEN starts the DMA. BUSY (bit 30) goes high.
        #   FULL (bit 31) stays low (only one buffer queued).
        # ----------------------------------------------------------------
        ori(T1, ZERO, AUDIO_LEN),
        sw(T1, 0x0004, T0),                          # AI_LEN = 0x800
        ori(T1, ZERO, 1),
        sw(T1, 0x0008, T0),                          # AI_CONTROL = 1

        # ----------------------------------------------------------------
        # Step 7: Phase 1 capture -- 128 x AI_STATUS reads
        #   Expected: BUSY=1, FULL=0 throughout
        #   First few samples may show 0 due to AI clock latency (~2-3 cycles)
        # ----------------------------------------------------------------
        ori(T3, ZERO, 128),                          # loop counter
        # loop1: (5 instructions -- bne offset MUST stay -5)
        lw(T4, 0x000C, T0),                          # read AI_STATUS
        sw(T4, 0x0000, T2),                          # store to trace
        addiu(T2, T2, 4),                            # advance trace pointer
        addiu(T3, T3, -1),                           # decrement counter
        bne(T3, ZERO, LOOP_BACK),                    # branch to loop1 if not done
        NOP,

        # ----------------------------------------------------------------
        # Step 8: TRIGGER 2 -- enqueue buffer 2
        #   AI_DRAM_ADDR does NOT auto-advance -- set it explicitly.
        #   Writing AI_LEN fills second FIFO slot.
        #   FULL (bit 31) should go high immediately after this write.
        # ----------------------------------------------------------------
        lui(T1, (AUDIO2_PHYS >> 16) & 0xFFFF),
        ori(T1, T1, AUDIO2_PHYS & 0xFFFF),           # $t1 = 0x00100808
        sw(T1, 0x0000, T0),                          # AI_DRAM_ADDR = 0x00100808
        ori(T1, ZERO, AUDIO_LEN),
        sw(T1, 0x0004, T0),                          # AI_LEN = 0x800

        # ----------------------------------------------------------------
        # Step 9: Phase 2 capture -- 128 x AI_STATUS reads
        #   Expected: BUSY=1, FULL=1 at start
        #   At some point FULL drops (buffer 1 finishes, buffer 2 promotes)
        #   while BUSY stays high. This independent transition is what
        #   ares does not model correctly.
        # ----------------------------------------------------------------
        ori(T3, ZERO, 128),                          # loop counter
        # loop2: (5 instructions -- bne offset MUST stay -5)
        lw(T4, 0x000C, T0),                          # read AI_STATUS
        sw(T4, 0x0000, T2),                          # store to trace
        addiu(T2, T2, 4),                            # advance trace pointer
        addiu(T3, T3, -1),                           # decrement counter
        bne(T3, ZERO, LOOP_BACK),                    # branch to loop2 if not done
        NOP,

        # ----------------------------------------------------------------
        # Step 10: Write end sentinel and halt
        # ----------------------------------------------------------------
        lui(T1, (SENTINEL_END >> 16) & 0xFFFF),
        ori(T1, T1, SENTINEL_END & 0xFFFF),          # $t1 = 0xCAFEBABE
        sw(T1, 0x0000, T2),                          # trace[last] = SENTINEL_END
        HALT,                                        # infinite loop
        NOP,
    ]

    for i, instr in enumerate(mips):
        rom[0x1000 + (i*4) : 0x1004 + (i*4)] = struct.pack(">I", instr)

    output = "n64-ai-diag.z64"
    with open(output, "wb") as f:
        f.write(rom)

    print(f"Built: {output}")
    print(f"  44100 Hz: dacrate={dacrate}, bitrate={bitrate}")
    print(f"  Audio buffer 1: phys 0x{AUDIO1_PHYS:08X}, len 0x{AUDIO_LEN:X}")
    print(f"  Audio buffer 2: phys 0x{AUDIO2_PHYS:08X}, len 0x{AUDIO_LEN:X}")
    print(f"  Trace buffer:   0x{TRACE_BASE:08X} (1032 bytes)")
    print(f"")
    print(f"To read trace after halt:")
    print(f"  sc64deployer dump 0x00102000 1032 trace.bin")
    print(f"  Expected trace[0]:   0x{SENTINEL_START:08X}  (start sentinel)")
    print(f"  Expected trace[258]: 0x{SENTINEL_END:08X}  (end sentinel)")

    for exe in ["rn64crc.exe", "n64crc.exe"]:
        if os.path.exists(exe):
            subprocess.run([exe, "-u", output])
            print(f"CRC updated via {exe}")
            break
    else:
        print("")
        print("Warning: no CRC tool found.")
        print(f"Run: rn64crc -u {output}")

if __name__ == "__main__":
    build_diag_rom()
