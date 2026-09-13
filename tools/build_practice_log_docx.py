#!/usr/bin/env python3
"""Generate the separate student practice log docx."""

from __future__ import annotations

import shutil
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile


ROOT = Path(__file__).resolve().parents[1]
TEMPLATE = ROOT / "编译原理课程设计" / "12 学生实践日志.docx"
OUTPUT = ROOT / "report" / "practice-log.docx"

W = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
NS = {"w": W}
ET.register_namespace("w", W)


LOG_ROWS = [
    ("第一次课", "安装实验所需的各种软件系统，并分析 C-- 语言。", "朱雪峰"),
    ("第二次课", "实践 1.1：学习 Flex，用 Flex 实现词法分析。", "朱雪峰"),
    ("第三次课", "实践 1.2：学习 Bison，用 Bison 实现语法分析。", "朱雪峰"),
    ("第四次课", "实践 2：进行语义分析，设计符号表和类型检查流程。", "朱雪峰"),
    ("第五次课", "实践 3：生成中间代码，并使用 IRSim 验证中间代码。", "朱雪峰"),
    ("第六次课", "实践 4：生成 MIPS32 目标代码，并使用 SPIM 运行验证。", "朱雪峰"),
    ("第七次课", "实践 5：进行代码优化，完成常量折叠、公共子表达式消除和无用代码删除。", "朱雪峰"),
    ("第八次课", "评优答辩，课程设计报告撰写与总结。", "朱雪峰"),
]


def wt(name: str) -> str:
    return f"{{{W}}}{name}"


def set_cell_text(cell: ET.Element, text: str) -> None:
    paragraphs = cell.findall("w:p", NS)
    if not paragraphs:
        p = ET.SubElement(cell, wt("p"))
    else:
        p = paragraphs[0]
        for child in list(p):
            p.remove(child)
    run = ET.SubElement(p, wt("r"))
    text_el = ET.SubElement(run, wt("t"))
    text_el.text = text
    for extra in paragraphs[1:]:
        cell.remove(extra)


def replace_paragraph_text(root: ET.Element, old_prefix: str, new_text: str) -> None:
    for p in root.findall(".//w:p", NS):
        text = "".join(t.text or "" for t in p.findall(".//w:t", NS)).strip()
        if text.startswith(old_prefix):
            for child in list(p):
                p.remove(child)
            run = ET.SubElement(p, wt("r"))
            text_el = ET.SubElement(run, wt("t"))
            text_el.text = new_text
            return


def main() -> None:
    with ZipFile(TEMPLATE, "r") as zin:
        root = ET.fromstring(zin.read("word/document.xml"))
        replace_paragraph_text(
            root,
            "学生姓名：",
            "学生姓名：郭宇杰    学号：        班级：23-3    课程名称：编译原理课程设计",
        )

        rows = root.findall(".//w:tr", NS)
        if len(rows) < len(LOG_ROWS) + 1:
            raise RuntimeError("practice log template does not have enough rows")

        for row, values in zip(rows[1:], LOG_ROWS):
            cells = row.findall("w:tc", NS)
            if len(cells) < 3:
                raise RuntimeError("practice log row has fewer than 3 cells")
            for cell, value in zip(cells[:3], values):
                set_cell_text(cell, value)

        new_document = ET.tostring(root, encoding="utf-8", xml_declaration=True)

        OUTPUT.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(delete=False, suffix=".docx", dir=str(OUTPUT.parent)) as tmpf:
            tmp_path = Path(tmpf.name)
        try:
            with ZipFile(tmp_path, "w", ZIP_DEFLATED) as zout:
                for item in zin.infolist():
                    data = zin.read(item.filename)
                    if item.filename == "word/document.xml":
                        data = new_document
                    zout.writestr(item, data)
            shutil.move(tmp_path, OUTPUT)
        finally:
            if tmp_path.exists():
                tmp_path.unlink()

    print(OUTPUT)


if __name__ == "__main__":
    main()
