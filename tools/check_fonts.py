#!/usr/bin/env python3
"""
Проверка покрытия TTF/OTF-шрифта нужными Unicode-диапазонами.

Примеры:
    python3 tools/check_font.py assets/fonts/main.ttf
    python3 tools/check_font.py assets/fonts/main.ttf --require cyrillic
    python3 tools/check_font.py assets/fonts/main.ttf --dump  # все диапазоны
    python3 tools/check_font.py assets/fonts/*.ttf --require latin cyrillic

Код возврата:
    0 — все запрошенные диапазоны покрыты
    1 — что-то отсутствует
    2 — ошибка (файл не найден, не TTF и т.п.)
"""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


# ---------------------------------------------------------------------------
#  Диапазоны, которые нас интересуют. Можно расширять.
# ---------------------------------------------------------------------------
@dataclass(frozen=True)
class Range:
    name: str
    start: int
    end: int

    def __contains__(self, cp: int) -> bool:
        return self.start <= cp <= self.end

    def __len__(self) -> int:
        return self.end - self.start + 1


RANGES: dict[str, Range] = {
    "ascii":       Range("ascii",       0x0020, 0x007E),
    "latin-1":     Range("latin-1",     0x00A0, 0x00FF),
    "cyrillic":    Range("cyrillic",    0x0400, 0x04FF),
    "cyrillic-supp": Range("cyrillic-supp", 0x0500, 0x052F),
    "punctuation": Range("punctuation", 0x2000, 0x206F),
    "arrows":      Range("arrows",      0x2190, 0x21FF),
    "math":        Range("math",        0x2200, 0x22FF),
    "box-drawing": Range("box-drawing", 0x2500, 0x257F),
    "emoji-misc":  Range("emoji-misc",  0x2600, 0x26FF),
}


# ---------------------------------------------------------------------------
#  Разбор аргументов
# ---------------------------------------------------------------------------
def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Проверка покрытия шрифта Unicode-диапазонами.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument("fonts", nargs="+", type=Path,
                   help="Пути к .ttf/.otf файлам")
    p.add_argument("--require", nargs="*", metavar="RANGE",
                   choices=sorted(RANGES.keys()),
                   default=["ascii", "cyrillic"],
                   help="Какие диапазоны обязательны (по умолчанию: ascii cyrillic)")
    p.add_argument("--dump", action="store_true",
                   help="Показать все известные диапазоны с %% покрытия")
    p.add_argument("--verbose", "-v", action="store_true",
                   help="Печатать отсутствующие код-поинты (первые 20 на диапазон)")
    p.add_argument("--sample", action="store_true",
                   help="Печатать строку-образец с доступными символами")
    return p.parse_args()


# ---------------------------------------------------------------------------
#  Ядро
# ---------------------------------------------------------------------------
def load_cmap(path: Path) -> dict[int, str]:
    """Возвращает {codepoint: glyph_name}. Бросает ImportError/RuntimeError."""
    try:
        from fontTools.ttLib import TTFont  # noqa: WPS433
    except ImportError as e:
        raise ImportError(
            "нужен пакет fonttools: pip install fonttools"
        ) from e

    if not path.exists():
        raise FileNotFoundError(f"файл не найден: {path}")
    if path.suffix.lower() not in {".ttf", ".otf", ".ttc"}:
        raise ValueError(f"не похоже на шрифт: {path}")

    try:
        font = TTFont(str(path), fontNumber=0, lazy=True)
    except Exception as e:
        raise RuntimeError(f"не удалось открыть {path}: {e}") from e

    # getBestCmap() возвращает лучший Unicode cmap, игнорируя symbol-таблицы.
    cmap = font.getBestCmap()
    if not cmap:
        raise RuntimeError(f"{path}: пустой cmap (нет Unicode-таблицы)")
    return cmap


def coverage(cmap: dict[int, str], rng: Range) -> tuple[int, int, list[int]]:
    """Возвращает (есть, всего, список_отсутствующих)."""
    present = [cp for cp in range(rng.start, rng.end + 1) if cp in cmap]
    missing = [cp for cp in range(rng.start, rng.end + 1) if cp not in cmap]
    return len(present), len(rng), missing


def format_cp(cp: int) -> str:
    ch = chr(cp)
    printable = ch if ch.isprintable() else "·"
    return f"U+{cp:04X} '{printable}'"


# ---------------------------------------------------------------------------
#  Вывод
# ---------------------------------------------------------------------------
GREEN = "\033[32m"
YELLOW = "\033[33m"
RED = "\033[31m"
RESET = "\033[0m"


def colorize(text: str, code: str, enabled: bool) -> str:
    return f"{code}{text}{RESET}" if enabled else text


def report(path: Path, cmap: dict[int, str], wanted: Iterable[str],
           dump: bool, verbose: bool, sample: bool) -> bool:
    use_color = sys.stdout.isatty()
    print(f"\n=== {path} ===")
    print(f"  glyphs в cmap: {len(cmap)}")

    ok = True
    if dump:
        for name, rng in RANGES.items():
            have, total, _ = coverage(cmap, rng)
            pct = 100.0 * have / total if total else 0.0
            bar_color = GREEN if pct >= 99.9 else (YELLOW if pct > 0 else RED)
            line = f"  {name:<16} {have:>5}/{total:<5} {pct:5.1f}%"
            print(colorize(line, bar_color, use_color))

    wanted_set = set(wanted)
    for name in sorted(wanted_set):
        rng = RANGES[name]
        have, total, missing = coverage(cmap, rng)
        pct = 100.0 * have / total if total else 0.0
        if have == total:
            tag = colorize("OK", GREEN, use_color)
        elif have == 0:
            tag = colorize("MISSING", RED, use_color)
            ok = False
        else:
            tag = colorize("PARTIAL", YELLOW, use_color)
            ok = False

        print(f"  [{tag}] {name:<16} {have}/{total} ({pct:.1f}%)")

        if verbose and missing:
            shown = missing[:20]
            more = len(missing) - len(shown)
            print("      missing: " +
                  ", ".join(format_cp(cp) for cp in shown) +
                  (f", … (+{more})" if more > 0 else ""))

    if sample:
        samples = build_sample(cmap)
        if samples:
            print("  sample:")
            for s in samples:
                print(f"    {s!r}")

    return ok


def build_sample(cmap: dict[int, str]) -> list[str]:
    """Строки-образцы из доступных код-поинтов."""
    out: list[str] = []

    ascii_str = "".join(
        chr(cp) for cp in range(0x20, 0x7F) if cp in cmap
    )
    if ascii_str:
        out.append(ascii_str)

    cyr = "".join(
        chr(cp) for cp in range(0x0410, 0x0450) if cp in cmap
    )
    if cyr:
        out.append(cyr)
        out.append(cyr)

    return out


# ---------------------------------------------------------------------------
#  Entry point
# ---------------------------------------------------------------------------
def main() -> int:
    args = parse_args()

    exit_code = 0
    for path in args.fonts:
        try:
            cmap = load_cmap(path)
        except ImportError as e:
            print(f"ошибка: {e}", file=sys.stderr)
            return 2
        except (FileNotFoundError, ValueError, RuntimeError) as e:
            print(f"ошибка: {e}", file=sys.stderr)
            exit_code = 2
            continue

        ok = report(path, cmap, args.require, args.dump, args.verbose, args.sample)
        if not ok:
            exit_code = max(exit_code, 1)

    return exit_code


if __name__ == "__main__":
    sys.exit(main())