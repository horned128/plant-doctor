#!/usr/bin/env python3
"""Run pyOCD with compatibility fixes required by the local debug probe."""

import sys

from pyocd.__main__ import main


NXP_MCULINK_ID = (0x1FC9, 0x0143)
MCULINK_CMSIS_DAP_USAGE_PAGE = 0xFFEB


def allow_mculink_hid_interface() -> None:
    """Accept the alternate HID usage page used by MCU-Link firmware V3.172."""
    from pyocd.probe.pydapaccess.interface import hidapi_backend

    original_filter = hidapi_backend.filter_device_by_usage_page

    def filter_device_by_usage_page(vid: int, pid: int, usage_page: int) -> bool:
        if (
            (vid, pid) == NXP_MCULINK_ID
            and usage_page == MCULINK_CMSIS_DAP_USAGE_PAGE
        ):
            return False
        return original_filter(vid, pid, usage_page)

    hidapi_backend.filter_device_by_usage_page = filter_device_by_usage_page


if __name__ == "__main__":
    allow_mculink_hid_interface()
    sys.argv[0] = "pyocd"
    raise SystemExit(main())
