"""Offline maps corroborated by the original 2026-10-08 acquisition reports.

These are carvings, NOT flash/boot instructions. End offsets are exclusive.
"""

PROFILES = {
    "t34-4g-km01": {
        "sha256": "55765bfd352f3101bc71204d537d6d216e42d2dd6d209f98753c2dd5960ac71d",
        "identity": "Anyka AK3918AV100 / AliOS Things (not Ingenic T34)",
        "basis": "CRC-valid saved environments; conflicting DTB map not used; runtime-unconfirmed",
        "partitions": [
            ("SPL", 0, 0x10000), ("ENV", 0x10000, 0x11000),
            ("FLAG", 0x11000, 0x12000), ("UBOOT", 0x12000, 0x40000),
            ("DTB", 0x40000, 0x50000), ("KERNEL", 0x50000, 0x250000),
            ("ROOTFS", 0x250000, 0x77e000), ("CONFIG", 0x77e000, 0x7fe000),
            ("FACTORY", 0x7fe000, 0x800000),
        ],
        "uimages": [("uboot", 0x12000, 0x40000), ("alios", 0x50000, 0x250000)],
        "dtbs": [0x7010, 0x3c1e0, 0x40000],
        "environments": [(0x10000, 0x1000), (0x11000, 0x1000)],
        "romfs": (0x250000, 0x77e000),
        "littlefs": (0x77e000, 0x7fe000),
    },
    "qc07f": {
        "sha256": "42f50a345b17e2c5fa00cab2b1a29bd900b793d5bbb8f3c0d9e2b41e603b2ba3",
        "identity": "Anyka AK3918EV200 / Linux 3.4.35",
        "basis": "Compiled default bootargs plus observed filesystem/kernel extents; runtime-unconfirmed; ENV text inconsistent",
        "partitions": [
            ("boot", 0, 0x60000), ("ENV", 0x60000, 0x70000),
            ("MAC", 0x70000, 0x80000), ("KERNEL", 0x80000, 0x200000),
            ("ROOT", 0x200000, 0x300000), ("USR.SQUASH", 0x300000, 0x780000),
            ("USR.JFFS2", 0x780000, 0x800000),
        ],
        "uimages": [("linux", 0x80000, 0x200000)],
        "xz_in_payload": 0x3868,
        "squashfs": [("root", 0x200000, 0x300000), ("usr", 0x300000, 0x780000)],
    },
    "fulhan-fh8626v200": {
        "sha256": "c7eda7587953a868064bab58857139c3a6a9edfcbc9937543766146b0c813fc0",
        "identity": "Fulhan FH8626V200 / Linux 4.9.129",
        "basis": "CRC-valid saved environment plus observed filesystem/kernel extents; runtime-unconfirmed",
        "partitions": [
            ("bootstrap", 0, 0x10000), ("uboot-env", 0x10000, 0x20000),
            ("uboot", 0x20000, 0x60000), ("kernel", 0x60000, 0x2b0000),
            ("data", 0x2b0000, 0x330000), ("res", 0x330000, 0x3b0000),
            ("app", 0x3b0000, 0x800000),
        ],
        "uimages": [("linux", 0x60000, 0x2b0000)],
        "environments": [(0x10000, 0x10000)],
        "xz_in_payload": 0x3774,
        "squashfs": [("app", 0x3b0000, 0x800000)],
    },
}

EXPECTED_SIZE = 8388608
