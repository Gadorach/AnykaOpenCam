# SPDX-License-Identifier: GPL-2.0-only
"""Pure exact observed-profile facts, never capture authenticity or admission.

Grammar: encoder-query77-cold-grammar-20261022/OBSERVED_PROFILE.md.
Read the v2 CONTRACT.md and README.md. No IO, CLI, callbacks or old imports.
"""
from dataclasses import dataclass
from enum import Enum


_SPEC_SHA256 = "daff4d31fcb034354df154fd0c5233824738150ee84ab59f9b862bf0f8c85437"
_MAX_INPUT_BYTES = 65536

# One symbolic production, not an assembled transcript or replay fixture.
# Every record except PROMPT includes its exact CRLF ending.
_RECORDS = (
    ("LEADING_EOL_1", b"\r\n"),
    ("LEADING_EOL_2", b"\r\n"),
    ("BANNER", b"U-Boot 2013.10.0-AK_V2.0.03 (Mar 15 2023 - 16:15:35)\r\n"),
    ("BANNER_SEPARATOR", b"\r\n"),
    ("DRAM", b"DRAM:  64 MiB\r\n"),
    ("SPI_FIRST", b"anyka spi flash. bus_width:1\r\n"),
    ("FLASH", b"8 MiB\r\n"),
    ("MMC_INIT", b"mmc_initialize: ak_sdhsmmc_init OK!\r\n"),
    ("MMC_HOST", b"ANYKA SDHC/MMC4.0: 0\r\n"),
    ("SPI_SECOND", b"anyka spi flash. bus_width:1\r\n"),
    ("CONSOLE_IN", b"In:    serial\r\n"),
    ("CONSOLE_OUT", b"Out:   serial\r\n"),
    ("CONSOLE_ERR", b"Err:   serial\r\n"),
    ("LOAD", b"load_addr = 0x81808000!\r\n"),
    ("BOARD", b"board_version: 5\r\n"),
    ("NET", b"Net:   eth_initialize board_version:5\r\n"),
    ("CHALLENGE", b"Please enter password:\r\n"),
    ("PROMPT_SEPARATOR", b"\r\n"),
    ("PROMPT", b"anyka$"),
)


class Classification(Enum):
    EXACT_OBSERVED_PROFILE_NOT_ADMISSION = "EXACT_OBSERVED_PROFILE_NOT_ADMISSION"
    PROPER_PREFIX_INCOMPLETE = "PROPER_PREFIX_INCOMPLETE"
    UNSUPPORTED_CONTEXT = "UNSUPPORTED_CONTEXT"


class ErrorCode(Enum):
    INPUT_TYPE = "COLD_CONTEXT_INPUT_TYPE"
    INPUT_TOO_LARGE = "COLD_CONTEXT_INPUT_TOO_LARGE"


class ColdContextError(Exception):
    """Fixed safe code/message fields; no caller bytes assigned to those fields."""
    def __init__(self, code):
        selected = code if type(code) is ErrorCode else ErrorCode.INPUT_TYPE
        self.code = selected
        super().__init__(selected.value)


@dataclass(frozen=True, slots=True)
class TranscriptFacts:
    classification: Classification
    matched_records: int
    matched_record_bytes: int
    challenge_record_matched: bool


def classify_cold_context(data: bytes) -> TranscriptFacts:
    """Classify ALL supplied bytes against only the pinned observed production.

    matched_record_bytes is the boundary after complete matched records, not a
    first-difference offset or permitted restart point. The input is not retained.
    Caller authenticates the complete arm+ON interval separately; text cannot
    prove actual origin, ownership, catch coverage, later quiet or a stable hold.
    """
    if type(data) is not bytes:
        raise ColdContextError(ErrorCode.INPUT_TYPE) from None
    if len(data) > _MAX_INPUT_BYTES:
        raise ColdContextError(ErrorCode.INPUT_TOO_LARGE) from None

    offset, matched, challenge = 0, 0, False
    for name, literal in _RECORDS:
        if len(data) - offset < len(literal):
            # This suffix copy is strictly shorter than ONE small record; never
            # copy a complete capture or skip bytes to find a later token.
            status = (Classification.PROPER_PREFIX_INCOMPLETE
                      if literal.startswith(data[offset:])
                      else Classification.UNSUPPORTED_CONTEXT)
            return TranscriptFacts(status, matched, offset, challenge)
        if not data.startswith(literal, offset):
            return TranscriptFacts(Classification.UNSUPPORTED_CONTEXT,
                                   matched, offset, challenge)
        offset += len(literal)
        matched += 1
        if name == "CHALLENGE":
            challenge = True  # complete expected record, including CRLF, only

    status = (Classification.EXACT_OBSERVED_PROFILE_NOT_ADMISSION
              if offset == len(data) else Classification.UNSUPPORTED_CONTEXT)
    return TranscriptFacts(status, matched, offset, challenge)
