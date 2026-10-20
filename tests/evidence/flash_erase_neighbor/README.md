# W25Q64 Flash Erase Neighbor Test

## Purpose

Document physical-hardware validation of sector erase isolation for the W25Q64 external SPI flash on the STM32 Nucleo-F446RE.

The test programs two adjacent sectors with distinguishable patterns, erases sector A, and verifies that A is blank while the upper neighboring sector B retains its data. A deliberate negative control changes the erase target from A to B to verify that the test detects the incorrect operation.

## Test configuration

- **MCU board:** STM32 Nucleo-F446RE
- **External flash:** W25Q64, 8 MiB
- **Interface:** SPI1 using STM32 HAL
- **Baseline commit:** `00e5e66` (`Add flash erase neighbor test`)
- **Expected JEDEC ID:** `EF 40 17`
- **Sector size:** 4096 bytes

The test uses the shared flash-layout definitions:

- Sector A: `FLASH_TEST_SECTOR_A_ADDRESS`
- Sector B: `FLASH_TEST_SECTOR_B_ADDRESS`

These are adjacent sectors, with B immediately above A in the flash address space.

## Evidence files

| File | Description |
|---|---|
| `flash_neighbor_positive_00e5e66.txt` | Original successful hardware run |
| `flash_neighbor_negative_00e5e66.txt` | Hardware run with the deliberately incorrect erase target |
| `flash_neighbor_negative_diff_00e5e66.txt` | Post-run transcription documenting the one-line source change |

The negative-control diff file is a transcription reconstructed from the recorded change after the run. It is not an original captured Git diff.

## Test procedure

The test performs the following operations:

1. Read and verify the flash JEDEC ID.
2. Erase sector A and verify that it is blank.
3. Erase sector B and verify that it is blank.
4. Program the sector A pattern.
5. Program the sector B pattern.
6. Verify the complete programmed pattern in A.
7. Verify the complete programmed pattern in B.
8. Erase A and verify that all bytes in A are `0xFF`.
9. Verify that B retains its programmed pattern.
10. Report the final verdict.

Each sector verification covers all 4096 bytes, using 256-byte reads.

## Observed results

### 1. Original positive hardware run

The original run returned JEDEC ID `EF 40 17`.

Both sectors were successfully erased, programmed, and verified before the final erase. After sector A was erased:

- Sector A passed its blank check with zero mismatches.
- Sector B passed its pattern check with zero mismatches.
- The final verdict was `PASS: Sector A erased; upper neighbor B intact`.

### 2. Negative-control hardware run

For the negative control, only the address argument in the step 8 erase call was changed:

```c
// Baseline
Flash_SectorErase(FLASH_TEST_SECTOR_A_ADDRESS, &hspi1)

// Negative control
Flash_SectorErase(FLASH_TEST_SECTOR_B_ADDRESS, &hspi1)
```

The setup erases, programming operations, pattern generation, verification logic, and verdict logic were left unchanged.

The observed results matched the predictions:

| Check | First mismatch | Total mismatches |
|---|---|---:|
| A post-erase blank check | Offset `0x000`: expected `0xFF`, actual `0x00` | 4096 / 4096 |
| B post-erase pattern check | Offset `0x000`: expected `0x70`, actual `0xFF` | 4096 / 4096 |

Steps 2 through 7 passed. Both post-erase checks executed, and the final verdict was `FAIL`.

This demonstrates that the test detects the deliberate wrong-sector erase through both verification modes and does not incorrectly report success.

The observation also provides limited evidence about downward erase spill: during this negative-control run, erasing B left all 4096 bytes of the lower neighboring sector A non-`0xFF`. Thus, the tested erase of B did not erase sector A.

### 3. Restored positive hardware run

The negative-control change was reverted. The restored `Core/Src/main.c` was rebuilt, and the build output showed the `main.c` object being recompiled. The firmware was flashed to the board, and the rerun reproduced the original successful result:

- Sector A post-erase blank check: zero mismatches.
- Sector B post-erase pattern verification: zero mismatches.
- Final verdict: `PASS: Sector A erased; upper neighbor B intact`.

The original positive, negative-control, and restored positive results are kept as separate evidence files.

## Limitations

### Neighbor coverage

The primary positive test erases sector A and checks sector A plus its upper neighbor B. It does not check the lower neighbor of A.

The guard sector and log region below A were not programmed and were not verified after A's erase. Consequently, **this test does not establish that erasing A cannot spill downward into the guard sector or log region**.

The negative control additionally shows that the tested erase of B did not erase A. That is a separate observation and does not replace testing the lower neighbor of A.

### Pattern coverage

The test pattern repeats every seven bytes within a page. Consequently, certain shifts by multiples of seven bytes within a page may be invisible to the pattern comparison. The test must not be interpreted as detecting every possible data shift or corruption pattern.

### Scope of the evidence

These results are physical-hardware observations for the tested firmware and flash device. They do not establish behavior under every possible hardware fault, flash failure mode, or erase disturbance.

Host-side tests of flash-driver validation and recovery logic provide separate evidence. They are not substitutes for the physical-hardware results documented here.

## Conclusion

The positive hardware run, deliberate negative control, and restored positive run demonstrate that the neighbor-sector test can verify the intended erase of A while detecting a deliberate erase of B instead. The negative control failed with the expected mismatches, and the restored firmware passed again.

The test provides evidence for the specific sector pair and operations exercised. Lower-neighbor isolation for A remains unverified.
