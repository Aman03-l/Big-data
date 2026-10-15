#!/usr/bin/env python3
"""
Hapus komentar (// dan /* */) dari file kode C/C++/.ino.
Aman terhadap string literal: "//" di dalam tanda kutip TIDAK dihapus
(mis. "http://192.168.1.1" tetap utuh).

Pemakaian:
    python strip_comments.py file1.ino file2.ino ...
"""
import sys


def strip_comments(src: str) -> str:
    out = []
    i = 0
    n = len(src)
    state = "code"   # code | line | block | str | chr

    while i < n:
        c = src[i]
        nxt = src[i + 1] if i + 1 < n else ""

        if state == "code":
            if c == "/" and nxt == "/":
                state = "line"
                i += 2
                continue
            if c == "/" and nxt == "*":
                state = "block"
                i += 2
                continue
            if c == '"':
                state = "str"
                out.append(c)
                i += 1
                continue
            if c == "'":
                state = "chr"
                out.append(c)
                i += 1
                continue
            out.append(c)
            i += 1

        elif state == "line":
            if c == "\n":
                state = "code"
                out.append(c)
            i += 1

        elif state == "block":
            if c == "*" and nxt == "/":
                state = "code"
                i += 2
                continue
            # pertahankan newline supaya nomor baris tetap
            if c == "\n":
                out.append(c)
            i += 1

        elif state == "str":
            out.append(c)
            if c == "\\" and nxt:
                out.append(nxt)
                i += 2
                continue
            if c == '"':
                state = "code"
            i += 1

        elif state == "chr":
            out.append(c)
            if c == "\\" and nxt:
                out.append(nxt)
                i += 2
                continue
            if c == "'":
                state = "code"
            i += 1

    text = "".join(out)
    # rapikan baris yang jadi kosong (spasi/tab saja) akibat komentar
    lines = [ln.rstrip() for ln in text.split("\n")]
    # buang baris kosong beruntun di akhir file
    while lines and lines[-1] == "":
        lines.pop()
    return "\n".join(lines) + "\n"


def main():
    for path in sys.argv[1:]:
        with open(path, "r", encoding="utf-8") as f:
            src = f.read()
        result = strip_comments(src)
        # rapikan: maksimal 1 baris kosong beruntun
        lines = result.split("\n")
        clean = []
        blank = 0
        for ln in lines:
            if ln.strip() == "":
                blank += 1
                if blank <= 1:
                    clean.append("")
            else:
                blank = 0
                clean.append(ln)
        result = "\n".join(clean).strip() + "\n"
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(result)
        print(f"OK: {path}")


if __name__ == "__main__":
    main()
