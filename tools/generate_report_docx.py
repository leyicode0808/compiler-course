#!/usr/bin/env python3
"""Generate course report docx files from Markdown drafts.

The generator uses an existing docx as a package/template source so the
original course cover, relationships, fonts and media are preserved. Only
word/document.xml is replaced with: template cover + page break + Markdown
body converted to simple Word paragraphs.
"""

from __future__ import annotations

import argparse
import copy
import re
import shutil
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile


W = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
NS = {"w": W}
ET.register_namespace("w", W)


def wt(name: str) -> str:
    return f"{{{W}}}{name}"


def w_el(name: str, attrs: dict[str, str] | None = None) -> ET.Element:
    el = ET.Element(wt(name))
    if attrs:
        for key, value in attrs.items():
            el.set(wt(key), value)
    return el


def append(parent: ET.Element, name: str, attrs: dict[str, str] | None = None) -> ET.Element:
    el = w_el(name, attrs)
    parent.append(el)
    return el


def add_text_run(p: ET.Element, text: str, *, bold: bool = False, font: str | None = None, size: str = "22") -> None:
    run = append(p, "r")
    rpr = append(run, "rPr")
    if bold:
        append(rpr, "b")
    if font:
        append(rpr, "rFonts", {"ascii": font, "hAnsi": font, "eastAsia": font})
    append(rpr, "sz", {"val": size})
    append(rpr, "szCs", {"val": size})
    t = append(run, "t")
    if text.startswith(" ") or text.endswith(" "):
        t.set("{http://www.w3.org/XML/1998/namespace}space", "preserve")
    t.text = text


def make_paragraph(
    text: str = "",
    *,
    style: str | None = None,
    outline: str | None = None,
    bold: bool = False,
    center: bool = False,
    font: str | None = None,
    size: str = "22",
    before: str | None = None,
    after: str | None = None,
) -> ET.Element:
    p = w_el("p")
    ppr = append(p, "pPr")
    if style:
        append(ppr, "pStyle", {"val": style})
    if outline is not None:
        append(ppr, "outlineLvl", {"val": outline})
    if center:
        append(ppr, "jc", {"val": "center"})
    if before or after:
        attrs = {}
        if before:
            attrs["before"] = before
        if after:
            attrs["after"] = after
        append(ppr, "spacing", attrs)
    if text:
        add_text_run(p, text, bold=bold, font=font, size=size)
    return p


def make_code_paragraph(text: str) -> ET.Element:
    return make_paragraph(text, font="Consolas", size="20", before="0", after="0")


def make_page_break() -> ET.Element:
    p = w_el("p")
    run = append(p, "r")
    append(run, "br", {"type": "page"})
    return p


def make_toc_field() -> ET.Element:
    p = w_el("p")

    run = append(p, "r")
    append(run, "fldChar", {"fldCharType": "begin"})

    run = append(p, "r")
    instr = append(run, "instrText")
    instr.set("{http://www.w3.org/XML/1998/namespace}space", "preserve")
    instr.text = 'TOC \\o "1-3" \\h \\z \\u'

    run = append(p, "r")
    append(run, "fldChar", {"fldCharType": "separate"})

    add_text_run(p, "目录将在 Word/WPS 中更新域后生成。", size="22")

    run = append(p, "r")
    append(run, "fldChar", {"fldCharType": "end"})
    return p


def parse_markdown(md_path: Path) -> list[ET.Element]:
    paragraphs: list[ET.Element] = []
    in_code = False
    code_buf: list[str] = []

    def flush_code() -> None:
        nonlocal code_buf
        if not code_buf:
            return
        paragraphs.append(make_paragraph("", before="120", after="0"))
        for line in code_buf:
            paragraphs.append(make_code_paragraph(line if line else " "))
        paragraphs.append(make_paragraph("", before="0", after="120"))
        code_buf = []

    for raw in md_path.read_text(encoding="utf-8").splitlines():
        line = raw.rstrip()
        if line.startswith("```"):
            if in_code:
                flush_code()
                in_code = False
            else:
                in_code = True
                code_buf = []
            continue

        if in_code:
            code_buf.append(line)
            continue

        stripped = line.strip()
        if not stripped:
            paragraphs.append(make_paragraph(""))
            continue

        if stripped == "[[PAGEBREAK]]":
            paragraphs.append(make_page_break())
        elif stripped == "[[TOC]]":
            paragraphs.append(make_toc_field())
        elif stripped.startswith("# "):
            paragraphs.append(
                make_paragraph(
                    stripped[2:].strip(),
                    style="Title",
                    bold=True,
                    center=True,
                    size="32",
                    before="240",
                    after="240",
                )
            )
        elif stripped.startswith("## "):
            paragraphs.append(
                make_paragraph(
                    stripped[3:].strip(),
                    style="Heading1",
                    outline="0",
                    bold=True,
                    size="28",
                    before="240",
                    after="120",
                )
            )
        elif stripped.startswith("### "):
            paragraphs.append(
                make_paragraph(
                    stripped[4:].strip(),
                    style="Heading2",
                    outline="1",
                    bold=True,
                    size="24",
                    before="160",
                    after="80",
                )
            )
        elif stripped.startswith("#### "):
            paragraphs.append(
                make_paragraph(
                    stripped[5:].strip(),
                    style="Heading3",
                    outline="2",
                    bold=True,
                    size="22",
                    before="120",
                    after="60",
                )
            )
        elif stripped.startswith("##### "):
            paragraphs.append(
                make_paragraph(
                    stripped[6:].strip(),
                    style="Heading4",
                    outline="3",
                    bold=True,
                    size="22",
                    before="100",
                    after="40",
                )
            )
        elif re.match(r"^[-*]\s+", stripped):
            paragraphs.append(make_paragraph("• " + stripped[2:].strip()))
        else:
            paragraphs.append(make_paragraph(stripped))

    if in_code:
        flush_code()
    return paragraphs


def has_page_break(p: ET.Element) -> bool:
    for br in p.findall(".//w:br", NS):
        if br.get(wt("type")) == "page":
            return True
    return False


def split_template_body(root: ET.Element) -> tuple[list[ET.Element], ET.Element]:
    body = root.find("w:body", NS)
    if body is None:
        raise RuntimeError("word/document.xml has no body")

    children = list(body)
    sect_pr = children[-1] if children and children[-1].tag == wt("sectPr") else w_el("sectPr")

    cover: list[ET.Element] = []
    for child in children:
        if child.tag == wt("sectPr"):
            continue
        cover.append(copy.deepcopy(child))
        if child.tag == wt("p") and has_page_break(child):
            return cover, copy.deepcopy(sect_pr)

    raise RuntimeError("template document does not contain a page break after cover")


def make_compact_cover() -> list[ET.Element]:
    lines = [
        ("计算机科学与技术 专业", "26", False, "360", "240"),
        ("课程设计报告", "44", True, "360", "720"),
        ("课程设计名称：编译原理课程设计", "24", False, "240", "360"),
        ("学    院    人工智能学院", "24", False, "120", "120"),
        ("专    业    计算机", "24", False, "120", "120"),
        ("班    级     23-3", "24", False, "120", "120"),
        ("姓    名    郭宇杰", "24", False, "120", "120"),
        ("指导教师    朱雪峰", "24", False, "120", "240"),
    ]
    cover: list[ET.Element] = []
    for text, size, bold, before, after in lines:
        cover.append(
            make_paragraph(
                text,
                bold=bold,
                center=True,
                size=size,
                before=before,
                after=after,
            )
        )
    cover.append(make_page_break())
    return cover


def make_style(
    style_id: str,
    name: str,
    *,
    based_on: str = "a",
    next_style: str = "a",
    priority: str = "9",
    outline: str | None = None,
    size: str = "28",
) -> ET.Element:
    style = w_el("style", {"type": "paragraph", "styleId": style_id})
    append(style, "name", {"val": name})
    append(style, "basedOn", {"val": based_on})
    append(style, "next", {"val": next_style})
    append(style, "uiPriority", {"val": priority})
    append(style, "qFormat")
    ppr = append(style, "pPr")
    if outline is not None:
        append(ppr, "outlineLvl", {"val": outline})
    rpr = append(style, "rPr")
    append(rpr, "b")
    append(rpr, "sz", {"val": size})
    append(rpr, "szCs", {"val": size})
    return style


def ensure_heading_styles(styles_xml: bytes) -> bytes:
    root = ET.fromstring(styles_xml)
    existing = {s.get(wt("styleId")) for s in root.findall("w:style", NS)}
    required = [
        ("Title", "Title", None, "32", "10"),
        ("Heading1", "heading 1", "0", "28", "9"),
        ("Heading2", "heading 2", "1", "24", "9"),
        ("Heading3", "heading 3", "2", "22", "9"),
        ("Heading4", "heading 4", "3", "22", "9"),
    ]
    for style_id, name, outline, size, priority in required:
        if style_id not in existing:
            root.append(make_style(style_id, name, outline=outline, size=size, priority=priority))
    return ET.tostring(root, encoding="utf-8", xml_declaration=True)


def ensure_update_fields(settings_xml: bytes) -> bytes:
    root = ET.fromstring(settings_xml)
    update = root.find("w:updateFields", NS)
    if update is None:
        update = w_el("updateFields", {"val": "true"})
        root.insert(0, update)
    else:
        update.set(wt("val"), "true")
    return ET.tostring(root, encoding="utf-8", xml_declaration=True)


def generate(template_docx: Path, markdown: Path, output: Path, *, compact_cover: bool = False) -> None:
    with ZipFile(template_docx, "r") as zin:
        root = ET.fromstring(zin.read("word/document.xml"))
        cover, sect_pr = split_template_body(root)
        if compact_cover:
            cover = make_compact_cover()
        body = root.find("w:body", NS)
        assert body is not None
        body.clear()
        for el in cover:
            body.append(el)
        for el in parse_markdown(markdown):
            body.append(el)
        body.append(sect_pr)
        new_document = ET.tostring(root, encoding="utf-8", xml_declaration=True)

        output.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(delete=False, suffix=".docx", dir=str(output.parent)) as tmpf:
            tmp_path = Path(tmpf.name)

        try:
            with ZipFile(tmp_path, "w", ZIP_DEFLATED) as zout:
                for item in zin.infolist():
                    data = zin.read(item.filename)
                    if item.filename == "word/document.xml":
                        data = new_document
                    elif item.filename == "word/styles.xml":
                        data = ensure_heading_styles(data)
                    elif item.filename == "word/settings.xml":
                        data = ensure_update_fields(data)
                    zout.writestr(item, data)
            shutil.move(tmp_path, output)
        finally:
            if tmp_path.exists():
                tmp_path.unlink()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("markdown", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--template", type=Path, default=Path("report/practice1.1-lexer-report.docx"))
    parser.add_argument("--compact-cover", action="store_true")
    args = parser.parse_args()
    generate(args.template, args.markdown, args.output, compact_cover=args.compact_cover)


if __name__ == "__main__":
    main()
