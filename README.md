# STM32 Environmental Data Logger

A bare-board data logger on an STM32 Nucleo-F446RE. Every 250 ms it reads temperature, pressure and humidity from a BME280, packs the reading into a 16 byte CRC-protected record, and appends it to a W25Q64 SPI flash with readback verification. On every boot a recovery scan walks the log, skips corrupt records, and resumes at the correct slot and sequence number. When the log is full it stops writing and says so.

The behavior listed under Evidence ran on hardware, and the full-log boot was also checked at the SPI bus level with a USB logic analyzer. Some paths described in the design are covered only by host tests or have not fired yet. The limits section lists them.

## Hardware

| Part | Interface | Notes |
|---|---|---|
| STM32 Nucleo-F446RE | | HAL based, no RTOS |
| Bosch BME280 | I2C, address 0x76 | Chip ID 0x60 confirmed |
| Winbond W25Q64 (8 MB) | SPI1, Mode 0, 2.625 MHz | JEDEC ID `EF 40 17` confirmed |
| UART console | USART2, 115200 8-N-1 | Through the Nucleo ST-LINK |

## Building

Toolchain: arm-none-eabi-gcc 14.3.1 (the one bundled with STM32CubeIDE 2.2.0), CMake presets, OpenOCD for flashing.

```
cmake --preset Debug
cmake --build --preset Debug
```

A clean checkout builds without any local files. This was verified with a fresh `git worktree` build at ebd9595. Later commits were built in the normal working tree. The normal image and the provisioning image both compile with zero warnings. The normal Debug image uses about 28 KB of flash and 2.2 KB of RAM.

### Provisioning a new chip

The normal firmware never erases the log. To prepare a chip, set `PROVISION_LOG` to 1 near the top of `Core/Src/main.c`, build, and flash. That image erases the four log sectors, reads back all 16,384 bytes to confirm they are 0xFF, prints the result, and halts. Then set the flag back to 0 and flash the normal build.

A provisioning image left on the board erases the log on every reset. The only protection is that the flag defaults to 0. A normal boot always prints a `RECOVERY` line and a provisioning image never does, so the two are easy to tell apart on the console.

## Architecture

```
            boot
              │
     sensor + flash init
              │
   recovery scan of log region ──► SCAN_FULL ──► "RECOVERY FULL. WRITES DISABLED."  (halt)
              │                    SCAN_IO_ERROR ──► halt without writing
           SCAN_OK
              │
   ┌──► wait for next 250 ms tick
   │          │
   │   BME280 forced measurement
   │          │
   │   build 16 B record (hardware CRC)
   │          │
   │   append: blank check ► program ► readback ► classify ► compare
   │          │                                   │
   │       verified                          any failure ──► storage fault (halt, no retry)
   │          │
   │   advance slot and seq
   │          │
   └── log full? ── yes ──► "LOG FULL. WRITES STOPPED."  (halt)
```

The firmware is a single superloop. The sensor driver, flash driver, record builder and recovery scan are separate modules, and the scan reads flash through a callback so it can be tested on a PC without hardware.

## Record format

Each record is 16 bytes, stored little endian.

| Offset | Field | Type | Meaning |
|---|---|---|---|
| 0 | `seq` | u32 | Scheduled sample index |
| 4 | `pressure_pa` | u32 | Pressure in Pa |
| 8 | `temperature_centi_c` | i16 | Temperature in 0.01 °C |
| 10 | `humidity_centi_pct` | u16 | Relative humidity in 0.01 % |
| 12 | `crc` | u32 | CRC-32/MPEG-2 over bytes 0 to 11 |

The CRC is computed by the F446 hardware CRC unit. `HAL_CRC_Calculate` resets the unit before each call, so every record starts from the reset value. The stored CRCs of two records read back from flash (slot 0 and slot 0x3FF) were recomputed independently in software and matched.

The BME280 values come from Bosch's integer compensation formulas, including the 64 bit pressure path, checked against hand calculations. `SensorRecord_Build` range-checks the values before narrowing them and writes its output only on success.

Each slot is classified in this order:

1. **BLANK**: all 16 bytes are 0xFF.
2. **VALID**: the stored CRC matches the computed one.
3. **CORRUPT**: anything else, such as a write that was cut short.

## Flash layout

| Region | Address range | Size |
|---|---|---|
| Log | 0x7F9000 to 0x7FCFFF | 4 sectors, 16 KB, 1024 slots |
| Guard sector | 0x7FD000 to 0x7FDFFF | 4 KB, never written |
| Test sector A | 0x7FE000 to 0x7FEFFF | 4 KB, used by bring-up tests |
| Test sector B | 0x7FF000 to 0x7FFFFF | 4 KB, used by bring-up tests |

Slot `n` lives at `0x7F9000 + 16 × n`. The log base is derived from the top of the chip, and compile-time checks confirm that the log is a whole number of sectors, starts on a sector boundary, and ends where the layout says it does.

## Scan and append policy

**Recovery scan.** The scan walks the log in physical order from slot 0. Corrupt slots are skipped and never reused. The first blank slot ends the scan and becomes the append slot. The next sequence number is the last valid `seq` plus one, or 0 for an empty log. If every slot is occupied, the scan reports `SCAN_FULL`. A HAL read failure reports `SCAN_IO_ERROR`, and the firmware stops without writing.

**Append.** Before programming, the firmware reads the destination slot and requires all 16 bytes to be 0xFF. It programs the record once, reads it back into a separate buffer, requires the readback to classify as VALID, and compares it byte for byte with what was programmed. Only then does it advance the slot and sequence counters in RAM.

**Failures are terminal.** Any append failure prints the specific reason and puts the firmware in a storage fault state for the rest of that boot. It never retries, never reprograms, and never erases. A failed program leaves the slot in an uncertain state, and the next boot's scan is the right place to judge it, not a retry that might program over half-written bits.

**Sequence numbers are a schedule.** Samples are taken every 250 ms, start to start, using a signed tick comparison. If the loop falls behind, it skips the missed samples instead of firing them back to back, advances `seq` past them, and prints one `SAMPLES SKIPPED` line. So `seq` is the scheduled sample index, not a count of records. A failed append never leaves a gap, because the firmware halts and the next boot reuses that `seq` at the next slot. That case shows up as the slot number and the sequence number drifting apart. A gap with no CORRUPT slot at that position means samples were skipped. A gap with a CORRUPT slot there means a record that was verified at write time went bad later.

**No runtime erase.** The log is erased once at provisioning and stops when full. There is no wraparound. This keeps the write path simple and makes every erase a deliberate act.

**Console output.** Each cycle prints a `SAMPLE` line with the readings, then `APPEND VERIFIED slot=... seq=...`. The SAMPLE line is printed before the append, so a failed append appears as a SAMPLE line with no APPEND VERIFIED after it.

## Evidence

Console excerpts from the earlier hardware runs are in [`tests/689b536/evidence-excerpts.txt`](tests/689b536/evidence-excerpts.txt). They were pasted from the serial console during the runs, and the original log files and the 689b536 analyzer capture were lost. The capture for the SPI startup fix is kept in full in [`tests/3fa350d/`](tests/3fa350d/).

| Commit | What was shown on hardware |
|---|---|
| b49b12e | Provisioning erased and verified four sectors that held programmed records. Boots before it resumed at slots 2 and 3, and boots after it started from slot 0. |
| 169358e | 179 verified appends in one continuous run. A reset between cycles, then resume at slot 0xB5 with matching seq. Scan time 22 ms for 182 reads. |
| 689b536 | A reset at slot 0x2C0 (scan 86 ms for 705 reads). A fill to slot and seq 0x3FF ending in `LOG FULL. WRITES STOPPED.` No samples were skipped across the fill. A boot on the full log printed `RECOVERY status=SCAN_FULL append_slot=0xFFFFFFFF next_seq=0x00000400 elapsed_ticks=0x0000007D`, then `RECOVERY FULL. WRITES DISABLED.`, and never started the logger. |

Readings during the runs were about 21.7 °C, 99,260 Pa and 47 to 50 %RH, which is plausible for an indoor room in Columbus, Ohio.

**Logic analyzer check of the full-log boot.** The decoded capture holds 1024 read transactions. 1023 of them decode cleanly as a `03` read command, and none of them carries a write enable (`06`), sector erase (`20`) or page program (`02`) command. So the full-log boot only read flash. The last transaction addresses `0x7FCFF0` (slot 0x3FF), and its payload decodes to the same seq and readings as the last SAMPLE line. The first transaction addresses slot 0. The addresses in between were not checked one by one. The span from the first chip select fall to the last chip select rise is about 125.3 ms, which agrees with `elapsed_ticks`.

**A missed prediction.** Before the full-log run, I predicted a full scan would take 65 to 85 ms, based on about 61 µs of SPI clock time per read. It took 125 ms, about 122 µs per read. The partial scans had already pointed to about 124 ms, but I kept the original prediction on record to test it as written. The extra time per read has not yet been broken down between gaps on the bus and work in the firmware.

**A startup bug found while re-capturing the evidence.** Every capture showed the same oddity on the first SPI transaction after reset: SCK sat high before chip select fell, dipped at the chip select edge, then dropped low just before clocking began. The decoder read the dip as an extra clock edge, so that transaction decoded one bit off (`81 BF C8 00` instead of `03 7F 90 00`). The flash usually ignored it. But on one boot right after flashing through OpenOCD, the scan read slot 0 of a full log as blank, reported an empty log, and the logger tried to append at slot 0. The append's destination blank check found the slot programmed and refused to write, so no data was lost.

The cause is in the startup order. `HAL_SPI_Init` configures SPI1 but leaves it disabled, and `HAL_SPI_Transmit` enables it on the first transfer, after the firmware has already pulled chip select low. Until then the SCK pin is not driven to its Mode 0 idle level and floats high. The leading explanation is that the chip select edge coupled into the floating SCK line, and on that boot the flash counted the dip as a clock edge, shifted its command by one bit, and returned nothing.

The fix (3fa350d) enables SPI1 once right after initialization, while chip select is still high, so SCK is driven low before the flash is ever selected. After the fix, captures of both an OpenOCD boot and a reset-button boot show SCK low before chip select falls, no dip, a first transaction that decodes directly as `03 7F 90 00` returning slot 0's record, and the expected `SCAN_FULL` in 125 ms.

**Earlier bring-up work** includes logic analyzer captures of each W25Q64 command sequence, 29 on-board tests of the record builder, a board table test of the classifier with a mutation check, 11 host tests of the recovery scan, and a neighbor erase test on the test sectors with a negative control (evidence at commit `<TODO hash>`; the test code has since been removed).

**Firmware version.** The full-log evidence was captured on the 689b536 image. No binary hash was recorded, so the link between each run and its commit rests on flashing builds of committed source. Since then the code changed by removing functions that had no callers (checked with `git grep`), building the provisioning code only when `PROVISION_LOG` is set, and the SPI startup fix in 3fa350d. The fix does change the boot path, so the full-log boot was captured again on 3fa350d and gave the same result.

## Limits

These were not tested, or were tested only partly:

- **Power loss.** Resume was shown across resets between appends, never across a power cut during a flash program, and there is no USB-unplug power cycle run. Torn records were tested only as simulated records in host and board tests. The accurate claim is "recovers from torn records (simulated)", not "survives power loss".
- **Untested paths on hardware.** The CORRUPT skip has not run on real flash with the current code (an earlier board test passed, but that code was lost, and the 11 host tests cover the algorithm). The `READBACK MISMATCH` path and the `SAMPLES SKIPPED` path have never fired on hardware.
- **Address mapping.** Slot to address mapping was checked on the wire for slots 0 and 0x3FF only, not stepped across all 1024.
- **Addressing bugs.** The readback compare cannot catch a bug that sends both the program and the readback to the same wrong address. Only a later scan would.
- **Bad wiring.** The scan reports only failures that HAL reports. If SPI succeeds but returns garbage, the result is CORRUPT slots, not `SCAN_IO_ERROR`.
- **No timestamps.** Time is implied by `seq` at 250 ms per step. Reboots and downtime leave no trace in flash.
- **SPI startup fix.** The failing boot was seen once and was not captured on the analyzer, so the link between the SCK glitch and that failure is an explanation, not a direct observation. The fix is verified by the glitch disappearing on two boots, not by a long run of repeated boots. A single false blank still ends the scan early by design, and the append's blank check is the backstop for that case.
- **Analyzer setup.** The channel mapping was read from the waveforms, not traced on the wires, and the sample rate was not recorded.
- **Neighbor erase test.** Only the upper neighbor of sector A was checked, so a downward spill is unobserved. The test pattern cannot detect shifts that are a multiple of 7 bytes within a page, and its early exits printed no verdict.
- **Datasheet figures.** Timing numbers used during design (BME280 measurement time, page program and erase times) and the F446 CRC unit details have not been rechecked against the datasheets.
- **Metadata journal.** `metadata_journal.c` and `metadata_crc.c` are in the repository but are host tested only and not compiled into the firmware. That work is paused.
- **Provisioning safety.** A provisioning image left flashed erases the log on every reset.

## Future work

- FreeRTOS tasks for sensing and storage, with a queue between them
- A mutex around the SPI flash once more than one task can use it
- A UART command task (`status`, `dump`)
- DMA for the flash write, with the completion interrupt signaling the waiting task
- Log wraparound and wear leveling
- A real power-cut test during flash programming, and a power-cycle resume run
- Full UART logs kept alongside every capture
- A repeated-boot test that resets the board many times and checks every scan result
- Finishing the metadata journal
