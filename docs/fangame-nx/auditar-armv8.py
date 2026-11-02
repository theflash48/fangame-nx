#!/usr/bin/env python3
"""Read-only check of executable ELF words and matching NRO code (M006).

This detects the known SVE/SME/GCS words from M004/M005. It is not a
complete Arm ISA validator; inspect full objdump output as a separate gate.
"""

import pathlib
import struct
import sys


PATTERNS = (
    ("CNTD (SVE)", 0xFFFFFFE0, 0x04E0E3E0),
    ("STR ZA (SME)", 0xFFFFFFF0, 0xE1206200),
    ("LDR ZA (SME)", 0xFFFFFFF0, 0xE1006200),
    ("ADDSVL (SVE)", 0xFFFFFFFF, 0x04305A10),
    ("MRS SVCR (SME)", 0xFFFFFFE0, 0xD53B4240),
    ("MRS TPIDR2_EL0 (SME)", 0xFFFFFFE0, 0xD53BD0A0),
    ("MSR TPIDR2_EL0 (SME)", 0xFFFFFFE0, 0xD51BD0A0),
    ("SMSTART/SMSTOP (SME)", 0xFFFFF0FF, 0xD503407F),
    ("CHKFEAT GCS", 0xFFFFFFFF, 0xD503251F),
    ("MRS GCSPR_EL0 (GCS)", 0xFFFFFFE0, 0xD53B2520),
    ("GCSPOPM (GCS)", 0xFFFFFFE0, 0xD52B7720),
)


def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def u64(data, offset):
    return struct.unpack_from("<Q", data, offset)[0]


def main(elf_path, nro_path):
    elf = pathlib.Path(elf_path).read_bytes()
    nro = pathlib.Path(nro_path).read_bytes()
    if elf[:4] != b"\x7fELF" or elf[4:6] != b"\x02\x01" or u16(elf, 18) != 183:
        raise ValueError("se requiere un ELF64 AArch64 little endian")
    if nro[0x10:0x14] != b"NRO0":
        raise ValueError("falta NRO0 en el NRO")

    shoff, shentsize, shnum, shstr = u64(elf, 40), u16(elf, 58), u16(elf, 60), u16(elf, 62)
    name_off = shoff + shstr * shentsize
    names = elf[u64(elf, name_off + 24):u64(elf, name_off + 24) + u64(elf, name_off + 32)]
    hits = []
    sections = []
    for i in range(shnum):
        base = shoff + i * shentsize
        flags, address, offset, size = (u64(elf, base + j) for j in (8, 16, 24, 32))
        if not (flags & 0x4) or size == 0:
            continue
        start = u32(elf, base)
        end = names.find(b"\0", start)
        name = names[start:end].decode("utf-8", "replace")
        if offset + size > len(elf):
            raise ValueError(f"sección {name} fuera del ELF")
        sections.append((name, address, size))
        for pos in range(offset, offset + size - 3, 4):
            word = u32(elf, pos)
            for label, mask, value in PATTERNS:
                if word & mask == value:
                    hits.append((name, address + pos - offset, word, label))
                    break

    phoff, phentsize, phnum = u64(elf, 32), u16(elf, 54), u16(elf, 56)
    compared = 0
    mismatches = []
    for i in range(phnum):
        base = phoff + i * phentsize
        if u32(elf, base) != 1 or not (u32(elf, base + 4) & 1):
            continue
        fileoff, vaddr, size = u64(elf, base + 8), u64(elf, base + 16), u64(elf, base + 32)
        for delta in range(size):
            # elf2nro writes the NRO header over the first 0x80 bytes.
            if vaddr + delta < 0x80:
                continue
            if fileoff + delta >= len(elf) or vaddr + delta >= len(nro):
                raise ValueError("segmento ejecutable fuera de ELF/NRO")
            compared += 1
            if elf[fileoff + delta] != nro[vaddr + delta] and len(mismatches) < 20:
                mismatches.append(vaddr + delta)

    print(f"secciones ejecutables: {sections}")
    print(f"instrucciones conocidas incompatibles: {len(hits)}")
    for name, addr, word, label in hits:
        print(f"  {name} 0x{addr:x} 0x{word:08x} {label}")
    print(f"bytes de código ELF/NRO comparados: {compared}; primeras diferencias: {mismatches}")
    return 0 if not hits and not mismatches else 1


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("uso: auditar-armv8.py <elf> <nro>")
    raise SystemExit(main(sys.argv[1], sys.argv[2]))
