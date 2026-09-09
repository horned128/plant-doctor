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
#   3. adds the real programming FlashRegion at 0x10000000-0x1003FFFF.

import os
from pathlib import Path

from pyocd.core.memory_map import FlashRegion
from pyocd.target.pack.flash_algo import PackFlashAlgo

FLASH_BASE = 0x10000000
FLASH_SIZE = 0x00040000  # 256 KiB


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
