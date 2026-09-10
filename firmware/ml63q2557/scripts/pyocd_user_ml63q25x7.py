# pyOCD user script for ROHM ML63Q25x7 / ML63Q2557.
#
# The ROHM DFP describes a ROM execution alias at 0x00000000 and a flash
# programming algorithm at 0x10000000. The vendor FLM metadata itself reports
# flash_start=0x00000000, which causes pyOCD to create a second FlashRegion
# overlapping the ROM alias. That produces GDB's:
#   warning: Overlapping regions in memory map: ignoring
#
# This script:
#   1. removes only the erroneous pack-generated FlashRegion at 0x00000000,
#      while preserving the IROM1 ROM alias used for execution/debugging;
#   2. corrects the vendor FLM flash base to 0x10000000;
#   3. adds the real programming FlashRegion at 0x10000000-0x1003FFFF;
#   4. services the hardware watchdog around long debug flash operations.

import os
from pathlib import Path

from pyocd.core.memory_map import FlashRegion
from pyocd.core.target import Target
from pyocd.target.pack.flash_algo import PackFlashAlgo

FLASH_BASE = 0x10000000
FLASH_SIZE = 0x00040000  # 256 KiB
WDT_CONTROL = 0x40003000
WDT_MODE = 0x40003004
WDT_PERIOD_8_SECONDS = 0x03
WDT_WRITE_PENDING = 0x01


def _service_watchdog() -> None:
    target.write32(WDT_MODE, WDT_PERIOD_8_SECONDS)
    for _ in range(100):
        target.write32(WDT_CONTROL, 0x5A)
        if (target.read32(WDT_CONTROL) & WDT_WRITE_PENDING) != 0:
            break
    else:
        raise RuntimeError("ML63Q25x7 watchdog did not accept the first clear key")
    target.write32(WDT_CONTROL, 0xA5)


def _will_flash(_notification) -> None:
    _service_watchdog()


def _guard_flash_operations() -> None:
    for region in target.memory_map.regions:
        if not region.is_flash:
            continue
        flash = region.flash
        if flash is None:
            continue

        original_erase_all = flash.erase_all
        original_erase_sector = flash.erase_sector
        original_program_page = flash.program_page
        original_start_program_page = flash.start_program_page_with_buffer

        def watchdog_safe_erase_all(erase_all=original_erase_all):
            _service_watchdog()
            return erase_all()

        def watchdog_safe_erase_sector(address, erase_sector=original_erase_sector):
            _service_watchdog()
            return erase_sector(address)

        def watchdog_safe_program_page(
            address, data, program_page=original_program_page
        ):
            _service_watchdog()
            return program_page(address, data)

        def watchdog_safe_start_program_page(
            buffer_number,
            address,
            start_program_page=original_start_program_page,
        ):
            _service_watchdog()
            return start_program_page(buffer_number, address)

        flash.erase_all = watchdog_safe_erase_all
        flash.erase_sector = watchdog_safe_erase_sector
        flash.program_page = watchdog_safe_program_page
        flash.start_program_page_with_buffer = watchdog_safe_start_program_page


def _find_flm() -> Path:
    root = os.environ.get("ROHM_DFP_ROOT")
    if not root:
        # pyocd-macos.sh normally sets this to the project-local copied DFP.
        raise RuntimeError("ROHM_DFP_ROOT is not set")

    root_path = Path(root).expanduser().resolve()
    preferred = root_path / "Flash" / "ML63Q25x7.FLM"
    if preferred.is_file():
        return preferred

    matches = sorted(
        p for p in root_path.rglob("*")
        if p.is_file()
        and p.suffix.lower() == ".flm"
        and p.stem.lower() == "ml63q25x7"
    )
    if not matches:
        raise RuntimeError(f"ML63Q25x7.FLM was not found below: {root_path}")
    return matches[0]


def will_connect(board):
    flm_path = _find_flm()

    # Remove the bad pack-generated flash mapping at the execution alias.
    # Keep IROM1 (Rom) at 0x00000000 intact.
    removed = []
    for region in list(target.memory_map.regions):
        if region.is_flash and region.start == 0x00000000:
            removed.append(region.name)
            target.memory_map.remove_region(region)

    pack_algo = PackFlashAlgo(str(flm_path))
    original_start = pack_algo.flash_start
    pack_algo.flash_start = FLASH_BASE

    # Avoid duplicate custom mappings in case a long-lived session reloads.
    existing = target.memory_map.get_region_for_address(FLASH_BASE)
    if existing is not None and existing.start == FLASH_BASE:
        target.memory_map.remove_region(existing)

    target.memory_map.add_region(
        FlashRegion(
            name="ML63Q25x7_program_flash",
            start=FLASH_BASE,
            length=FLASH_SIZE,
            access="rx",
            is_default=True,
            is_testable=False,
            flm=pack_algo,
            sector_size=0,
        )
    )

    print(f"[ML63Q25x7 pyOCD] FLM: {flm_path}")
    if removed:
        print(f"[ML63Q25x7 pyOCD] Removed overlapping pack flash: {', '.join(removed)}")
    print(
        "[ML63Q25x7 pyOCD] FLM flash base override: "
        f"0x{original_start:08X} -> 0x{FLASH_BASE:08X}"
    )
    print(
        "[ML63Q25x7 pyOCD] Programming region: "
        f"0x{FLASH_BASE:08X}-0x{FLASH_BASE + FLASH_SIZE - 1:08X}"
    )


def did_connect(board):
    # Flash algorithms execute on the target and can take longer than the
    # application's 2-second watchdog period. Extend and service the watchdog
    # before pyOCD starts erase/program operations.
    _service_watchdog()
    board.session.subscribe(
        _will_flash,
        (Target.Event.PRE_FLASH_ERASE, Target.Event.PRE_FLASH_PROGRAM),
    )
    _guard_flash_operations()

    original_read_memory_block8 = target.read_memory_block8

    def watchdog_safe_read_memory_block8(address, size):
        _service_watchdog()
        return original_read_memory_block8(address, size)

    target.read_memory_block8 = watchdog_safe_read_memory_block8
    print("[ML63Q25x7 pyOCD] Watchdog extended to 8 seconds for this session")
