#!/usr/bin/env python3
"""
dirtree.py — дерево директорий с игнорированием ненужных папок.

Файлы и папки НИКОГДА не удаляются. Игнор-лист хранится в <root>/.dirtreeignore.

Примеры:
    python dirtree.py .                   # показать дерево (с учётом игнора)
    python dirtree.py . -s                # + размеры
    python dirtree.py . --show-ignored    # показать игнорируемые (приглушённо)
    python dirtree.py . --depth 3         # ограничить глубину
    python dirtree.py . -i                # интерактивно вкл/выкл игнор у папок
    python dirtree.py . --patterns        # показать активные правила игнора
"""

import argparse
import fnmatch
import os
import sys
from pathlib import Path

# ---------- ANSI цвета ----------
class C:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    DIM = "\033[2m"
    BLUE = "\033[34m"
    CYAN = "\033[36m"
    GREEN = "\033[32m"
    YELLOW = "\033[33m"
    RED = "\033[31m"

GLOBAL_CONFIG = Path.home() / ".config" / "dirtree" / "ignore"
LOCAL_FILE = ".dirtreeignore"

DEFAULT_PATTERNS = [
    ".git", ".idea", ".vscode", "__pycache__", "node_modules",
    ".venv", ".mypy_cache", ".pytest_cache", ".DS_Store", "*.pyc",
]


def human_size(n: int) -> str:
    for u in ("B", "KB", "MB", "GB", "TB"):
        if n < 1024:
            return f"{n:.1f} {u}" if u != "B" else f"{n} B"
        n /= 1024
    return f"{n:.1f} PB"


def dir_size(path: Path) -> int:
    total = 0
    for root, _dirs, files in os.walk(path, onerror=lambda e: None):
        for f in files:
            try:
                total += (Path(root) / f).stat().st_size
            except OSError:
                pass
    return total


# ---------- Игнор-лист ----------
def load_patterns(root: Path, use_defaults: bool = True):
    """Возвращает (global_patterns, local_patterns)."""
    global_pats = list(DEFAULT_PATTERNS) if use_defaults else []
    if GLOBAL_CONFIG.exists():
        try:
            for line in GLOBAL_CONFIG.read_text(encoding="utf-8").splitlines():
                s = line.strip()
                if s and not s.startswith("#"):
                    global_pats.append(s)
        except OSError:
            pass

    local_pats = []
    lf = root / LOCAL_FILE
    if lf.exists():
        try:
            for line in lf.read_text(encoding="utf-8").splitlines():
                s = line.strip()
                if s and not s.startswith("#"):
                    local_pats.append(s)
        except OSError:
            pass
    return global_pats, local_pats


def save_local_patterns(root: Path, patterns):
    lf = root / LOCAL_FILE
    if not patterns:
        if lf.exists():
            lf.unlink()
        return
    lf.write_text("\n".join(patterns) + "\n", encoding="utf-8")


def match_pattern(rel: str, name: str, pattern: str) -> bool:
    """Без '/' — совпадение по имени на любой глубине. С '/' — по относительному пути."""
    if not pattern:
        return False
    if "/" in pattern:
        return fnmatch.fnmatch(rel, pattern)
    return fnmatch.fnmatch(name, pattern)


def is_ignored(path: Path, root: Path, patterns) -> bool:
    try:
        rel = path.relative_to(root).as_posix()
    except ValueError:
        rel = path.name
    return any(match_pattern(rel, path.name, p) for p in patterns)


# ---------- Обход ----------
def iter_entries(path: Path, root: Path, patterns, show_ignored: bool):
    """Список (entry, is_dir, ignored) с учётом игнора."""
    try:
        entries = list(path.iterdir())
    except (PermissionError, OSError):
        return []

    dirs = sorted([e for e in entries if e.is_dir()], key=lambda p: p.name.lower())
    files = sorted([e for e in entries if e.is_file()], key=lambda p: p.name.lower())

    result = []
    for d in dirs:
        ig = is_ignored(d, root, patterns)
        if ig and not show_ignored:
            continue
        result.append((d, True, ig))
    for f in files:
        ig = is_ignored(f, root, patterns)
        if ig and not show_ignored:
            continue
        result.append((f, False, ig))
    return result


# ---------- Печать дерева ----------
def print_tree(root: Path, patterns, max_depth=None, show_sizes=False, show_ignored=False):
    root = root.resolve()
    print(f"{C.BOLD}{C.BLUE}{root}{C.RESET}")

    def walk(path: Path, prefix: str = "", depth: int = 0):
        if max_depth is not None and depth >= max_depth:
            return
        entries = iter_entries(path, root, patterns, show_ignored)
        for i, (entry, is_dir, ig) in enumerate(entries):
            last = i == len(entries) - 1
            branch = "└── " if last else "├── "
            ext = "    " if last else "│   "

            tag = f" {C.DIM}(ignored){C.RESET}" if ig else ""

            if is_dir:
                color = C.DIM if ig else C.CYAN
                size_str = ""
                if show_sizes and not ig:
                    try:
                        size_str = f" {C.DIM}[{human_size(dir_size(entry))}]{C.RESET}"
                    except Exception:
                        pass
                print(f"{prefix}{branch}{color}{C.BOLD}{entry.name}/{C.RESET}{size_str}{tag}")
                if not ig:  # не спускаемся в игнорируемые
                    walk(entry, prefix + ext, depth + 1)
            else:
                color = C.DIM if ig else ""
                size_str = ""
                if show_sizes:
                    try:
                        size_str = f" {C.DIM}[{human_size(entry.stat().st_size)}]{C.RESET}"
                    except OSError:
                        pass
                print(f"{prefix}{branch}{color}{entry.name}{C.RESET}{size_str}{tag}")

    walk(root)


# ---------- Интерактивное переключение игнора ----------
def parse_selection(text: str, max_n: int) -> set:
    chosen = set()
    text = text.strip()
    if not text:
        return chosen
    for chunk in text.replace(" ", "").split(","):
        if not chunk:
            continue
        if "-" in chunk:
            a, b = chunk.split("-", 1)
            try:
                a, b = int(a), int(b)
            except ValueError:
                raise ValueError(f"некорректный диапазон: {chunk}")
            if a > b:
                a, b = b, a
            for n in range(a, b + 1):
                if 1 <= n <= max_n:
                    chosen.add(n)
        else:
            try:
                n = int(chunk)
            except ValueError:
                raise ValueError(f"не число: {chunk}")
            if 1 <= n <= max_n:
                chosen.add(n)
    return chosen


def collect_all_dirs(root: Path, patterns, max_depth):
    """Все подпапки (включая игнорируемые) — (depth, path, ignored)."""
    result = []

    def walk(path: Path, depth: int):
        if max_depth is not None and depth >= max_depth:
            return
        try:
            subs = sorted([e for e in path.iterdir() if e.is_dir()], key=lambda p: p.name.lower())
        except (PermissionError, OSError):
            return
        for d in subs:
            ig = is_ignored(d, root, patterns)
            result.append((depth, d, ig))
            if not ig:
                walk(d, depth + 1)

    walk(root, 0)
    return result


def interactive_toggle(root: Path, use_defaults=True, max_depth=None):
    root = root.resolve()
    global_pats, local_pats = load_patterns(root, use_defaults)
    all_pats = global_pats + local_pats

    dirs = collect_all_dirs(root, all_pats, max_depth)
    if not dirs:
        print(f"{C.YELLOW}Подпапок нет.{C.RESET}")
        return

    print(f"{C.BOLD}Корень:{C.RESET} {C.BLUE}{root}{C.RESET}")
    print(f"{C.DIM}Отмеченные [ignored] не показываются в дереве. Локальный файл: "
          f"{root / LOCAL_FILE}{C.RESET}\n")

    for i, (depth, p, ig) in enumerate(dirs, 1):
        rel = p.relative_to(root).as_posix()
        indent = "  " * depth
        if ig:
            marker = f"{C.DIM}[ignored]{C.RESET}"
            color = C.DIM
        else:
            marker = "         "
            color = C.CYAN
        print(f"{C.YELLOW}{i:>4}{C.RESET} {marker} {indent}{color}{rel}/{C.RESET}")

    print(
        f"\n{C.BOLD}Введите номера, чтобы переключить игнор (вкл/выкл):{C.RESET}\n"
        f"{C.DIM}например: 1,3,5-8   |   'all'   |   Enter — выход{C.RESET}"
    )
    raw = input("> ").strip().lower()
    if not raw:
        return
    if raw == "all":
        chosen = set(range(1, len(dirs) + 1))
    else:
        try:
            chosen = parse_selection(raw, len(dirs))
        except ValueError as e:
            print(f"{C.RED}Ошибка: {e}{C.RESET}")
            return
    if not chosen:
        return

    local_set = list(local_pats)
    for n in sorted(chosen):
        _, p, ig = dirs[n - 1]
        rel = p.relative_to(root).as_posix()

        if ig:
            # убираем из локального игнора
            before = len(local_set)
            local_set = [pat for pat in local_set if not match_pattern(rel, p.name, pat)]
            removed = before != len(local_set)
            # проверим, не остался ли путь игнорируемым глобальным правилом
            still = any(match_pattern(rel, p.name, g) for g in global_pats)
            if still:
                print(
                    f"{C.YELLOW}⚠ {rel} всё ещё игнорируется глобальным правилом "
                    f"({GLOBAL_CONFIG}){C.RESET}"
                )
            elif removed:
                print(f"{C.GREEN}− убрано из игнора: {rel}{C.RESET}")
            else:
                print(f"{C.DIM}· {rel} не было в локальном игноре{C.RESET}")
        else:
            if rel not in local_set:
                local_set.append(rel)
            print(f"{C.GREEN}+ добавлено в игнор: {rel}{C.RESET}")

    save_local_patterns(root, local_set)
    print(f"\n{C.BOLD}Сохранено: {root / LOCAL_FILE}{C.RESET}")


def show_patterns(root: Path, use_defaults=True):
    root = root.resolve()
    global_pats, local_pats = load_patterns(root, use_defaults)
    print(f"{C.BOLD}Глобальные ({GLOBAL_CONFIG if GLOBAL_CONFIG.exists() else 'встроенные дефолты'}):{C.RESET}")
    for p in global_pats:
        print(f"  {C.DIM}{p}{C.RESET}")
    print(f"\n{C.BOLD}Локальные ({root / LOCAL_FILE}):{C.RESET}")
    if not local_pats:
        print(f"  {C.DIM}(пусто){C.RESET}")
    else:
        for p in local_pats:
            print(f"  {C.CYAN}{p}{C.RESET}")


# ---------- main ----------
def main():
    p = argparse.ArgumentParser(
        description="Дерево директорий с игнорированием ненужных папок (без удаления).",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument("path", nargs="?", default=".", help="корневой путь (по умолчанию '.')")
    p.add_argument("-d", "--depth", type=int, default=None, help="максимальная глубина")
    p.add_argument("-s", "--sizes", action="store_true", help="показывать размеры")
    p.add_argument("-i", "--interactive", action="store_true",
                   help="интерактивно вкл/выкл игнор у папок")
    p.add_argument("--show-ignored", action="store_true",
                   help="показывать игнорируемые (приглушённо, без обхода)")
    p.add_argument("--patterns", action="store_true", help="показать активные правила игнора")
    p.add_argument("--no-defaults", action="store_true",
                   help="не использовать встроенные дефолты (только .gitignore-подобный файл)")
    args = p.parse_args()

    root = Path(args.path).expanduser()
    if not root.exists():
        print(f"{C.RED}Путь не существует: {root}{C.RESET}")
        sys.exit(1)
    if not root.is_dir():
        print(f"{C.RED}Это не папка: {root}{C.RESET}")
        sys.exit(1)

    use_defaults = not args.no_defaults

    try:
        if args.patterns:
            show_patterns(root, use_defaults)
            return
        if args.interactive:
            interactive_toggle(root, use_defaults, args.depth)
            return
        global_pats, local_pats = load_patterns(root, use_defaults)
        print_tree(root, global_pats + local_pats, args.depth, args.sizes, args.show_ignored)
    except KeyboardInterrupt:
        print(f"\n{C.YELLOW}Прервано.{C.RESET}")
        sys.exit(130)


if __name__ == "__main__":
    main()