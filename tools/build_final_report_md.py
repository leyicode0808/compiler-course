#!/usr/bin/env python3
"""Assemble the final course design report Markdown."""

from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "report"


PRACTICES = [
    ("实践一 词法分析与语法分析", [
        REPORT / "practice1.1-lexer-report.md",
        REPORT / "practice1.2-parser-report.md",
    ]),
    ("实践二 语义分析", [REPORT / "practice2-semantic-report.md"]),
    ("实践三 中间代码生成", [REPORT / "practice3-ir-report.md"]),
    ("实践四 目标代码生成", [REPORT / "practice4-mips-report.md"]),
    ("实践五 中间代码优化", [REPORT / "practice5-opt-report.md"]),
]

TOC_PAGE_NUMBERS: dict[str, int] = {
    "2:1:实践一 词法分析与语法分析": 6,
    "3:1:编译原理课程设计报告：实践 1.1 Flex 词法分析": 6,
    "4:1:一、目的和任务": 6,
    "4:1:二、过程": 7,
    "4:1:三、结果与结论分析": 10,
    "3:1:编译原理课程设计报告：实践 1.2 Bison 语法分析": 12,
    "4:2:一、目的和任务": 12,
    "4:2:二、过程": 13,
    "4:2:三、结果与结论分析": 16,
    "2:1:实践二 语义分析": 18,
    "3:1:编译原理课程设计报告：实践 2 语义分析": 18,
    "4:3:一、目的和任务": 18,
    "4:3:二、过程": 19,
    "4:3:三、结果与结论分析": 22,
    "2:1:实践三 中间代码生成": 24,
    "3:1:编译原理课程设计报告：实践 3 中间代码生成": 24,
    "4:4:一、目的和任务": 25,
    "4:4:二、过程": 25,
    "4:4:三、结果与结论分析": 29,
    "2:1:实践四 目标代码生成": 31,
    "3:1:编译原理课程设计报告：实践 4 目标代码生成": 31,
    "4:5:一、目的和任务": 31,
    "4:5:二、过程": 32,
    "4:5:三、结果与结论分析": 34,
    "2:1:实践五 中间代码优化": 36,
    "3:1:编译原理课程设计报告：实践 5 中间代码优化": 36,
    "4:6:一、目的和任务": 36,
    "4:6:二、过程": 37,
    "4:6:三、结果与结论分析": 39,
    "2:1:工作总结": 41,
}


SUMMARY = """## 工作总结

本次课程设计从 C-- 源程序出发，逐步实现了词法分析、语法分析、语义分析、中间代码生成、目标代码生成和中间代码优化。项目最终形成了一个可以将 C-- 程序翻译为 MIPS32 汇编并在 SPIM 中运行的简易编译器，同时提供了独立的 IR 优化器。

实践一完成了 Flex 与 Bison 的前端基础，能够识别 C-- token、过滤注释、报告词法错误，并在语法正确时输出课程要求格式的语法树。实践二在语法树基础上实现类型系统和符号表，支持变量、函数、数组、结构体和作用域相关的语义检查。实践三把语义正确的程序翻译为三地址形式中间代码，并通过课程 IRSim 验证执行结果。实践四在 IR 基础上生成 MIPS32 汇编，完成函数栈帧、参数传递、递归调用、条件跳转、数组/结构体地址访问和读写系统调用。实践五实现保守的 IR 优化，包括常量传播、常量折叠、复制传播、局部公共子表达式消除和无用赋值删除，并保持优化前后运行结果一致。

整个实现过程中，核心原则是保持各阶段接口清晰：词法分析输出 token，语法分析构造 AST，语义分析检查 AST 并维护符号信息，中间代码生成把高级结构降为统一的线性 IR，目标代码生成只依赖 IR 完成 MIPS 翻译，优化器直接处理文本 IR。这样既便于分阶段测试，也便于在后续阶段复用前面阶段的结果。

当前项目已经通过实践 1-5 的本地回归测试：词法、语法、语义、IRSim、SPIM 和优化后 IRSim 验证均可执行。后续如果继续完善，可以重点改进寄存器分配、全局数据流优化、目标代码指令选择质量和更完整的异常输入处理。
"""


def toc_key(level: int, title: str, occurrence: int) -> str:
    return f"{level}:{occurrence}:{title}"


def downgrade(markdown: str) -> str:
    lines: list[str] = []
    for raw in markdown.splitlines():
        if raw.startswith("#"):
            hashes, _, title = raw.partition(" ")
            if set(hashes) == {"#"}:
                lines.append("#" * (len(hashes) + 2) + " " + title)
                continue
        if raw.startswith("课程设计名称："):
            continue
        lines.append(raw)
    return "\n".join(lines).strip()


def build_body_lines() -> list[str]:
    lines: list[str] = []
    for heading, paths in PRACTICES:
        lines.append(f"## {heading}")
        lines.append("")
        for path in paths:
            lines.extend(downgrade(path.read_text(encoding="utf-8")).splitlines())
            lines.append("")
    lines.extend(SUMMARY.strip().splitlines())
    lines.append("")
    return lines


def collect_toc_entries(lines: list[str]) -> list[tuple[int, str, str]]:
    entries: list[tuple[int, str, str]] = []
    occurrences: dict[tuple[int, str], int] = {}
    for raw in lines:
        if not raw.startswith("#"):
            continue
        hashes, _, title = raw.partition(" ")
        if not title or not set(hashes) == {"#"}:
            continue
        level = len(hashes)
        if level < 2 or level > 4:
            continue
        if title == "目录":
            continue
        item_key = (level, title)
        occurrences[item_key] = occurrences.get(item_key, 0) + 1
        entries.append((level, title, toc_key(level, title, occurrences[item_key])))
    return entries


def format_toc(lines: list[str]) -> list[str]:
    toc = ["## 目录", ""]
    for level, title, key in collect_toc_entries(lines):
        indent = "  " * (level - 2)
        dots = "." * max(8, 42 - len(indent) - len(title))
        page = TOC_PAGE_NUMBERS.get(key, "??")
        toc.append(f"{indent}{title} {dots} {page}")
        toc.append("")
    return toc


def main() -> None:
    body_lines = build_body_lines()
    parts: list[str] = [
        "# 编译原理课程设计报告",
        "",
        *format_toc(body_lines),
        "[[PAGEBREAK]]",
        "",
    ]
    parts.extend(body_lines)

    output = REPORT / "final-course-report.md"
    output.write_text("\n".join(parts), encoding="utf-8")
    print(output)


if __name__ == "__main__":
    main()
