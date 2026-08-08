#!/usr/bin/env python3
"""
生成全中文字库 (GB2312, 含常用标点) 供 ESP32-S3 LVGL 使用。
CI 中使用: 先 `npm i -g lv_font_conv`, 再运行本脚本生成 main/ui_fonts.c

输出说明:
- lv_font_conv 生成的是 lv_font_t 对象 (font_cn_16_obj / font_cn_22_obj)
- 为满足 ui.h 中的指针声明 `extern const lv_font_t *font_cn_16;`,
  本脚本把真实字体对象与指针别名一起写入 main/ui_fonts.c:
      const lv_font_t font_cn_16_obj = {...};
      const lv_font_t *font_cn_16 = &font_cn_16_obj;
- 若环境无中文字体或 lv_font_conv 失败, 则保留 main/ui_fonts.c 中的英文占位回退版,
  保证固件仍可编译烧录 (中文显示为英文/方框, 但不崩溃)。
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "ui_fonts.c")
GEN16 = os.path.join(HERE, "_ui_fonts_16_gen.c")
GEN22 = os.path.join(HERE, "_ui_fonts_22_gen.c")

# 优先使用系统字体, 否则下载思源黑体
FONT_PATHS = [
    "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/opentype/noto/NotoSansCJKsc-Regular.otf",
    "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
]


def find_font():
    for p in FONT_PATHS:
        if os.path.exists(p):
            return p
    return None


def run_conv(font, name, size, out_path, extra_ranges=None):
    cmd = [
        "lv_font_conv",
        "--font", font,
        "--format", "lvgl",
        "--name", name,
        "--size", str(size),
        "--bpp", "4",
        "--range", "0x20-0x7E",          # ASCII
        "--range", "0x3000-0x303F",       # CJK 符号
        "--range", "0x4E00-0x9FFF",       # CJK 统一表意文字 (常用汉字)
        "--range", "0xFF00-0xFFEF",       # 全角字符
        "--symbols", "🌐🎵📻🔍🎤▶⏸⏮⏭📑🎻🎷🎧🪕📰✨🌙📡🔄🔀🔁",
        "--output", out_path,
    ]
    if extra_ranges:
        cmd += extra_ranges
    subprocess.run(cmd, check=True)


def main():
    font = find_font()
    if not font:
        print("[gen_fonts] 未找到系统中文字体, 保留 main/ui_fonts.c 英文占位字体")
        return
    try:
        # 生成 16px 真实字体对象 -> 临时文件
        run_conv(font, "font_cn_16_obj", 16, GEN16)
        # 生成 22px 真实字体对象 -> 临时文件
        run_conv(font, "font_cn_22_obj", 22, GEN22)

        # 合并为 ui_fonts.c: 对象 + 指针别名, 与 ui.h 指针声明一致
        with open(GEN16, "r", encoding="utf-8") as f:
            body16 = f.read()
        with open(GEN22, "r", encoding="utf-8") as f:
            body22 = f.read()

        alias = (
            "\n/* 指针别名: 供 ui.h 中 extern const lv_font_t *font_cn_* 使用 */\n"
            "const lv_font_t *font_cn_16 = &font_cn_16_obj;\n"
            "const lv_font_t *font_cn_22 = &font_cn_22_obj;\n"
        )
        with open(OUT, "w", encoding="utf-8") as f:
            f.write("/* 自动生成: 全中文字库 (lv_font_conv). 勿手动编辑. */\n")
            f.write('#include "lvgl.h"\n\n')
            f.write(body16)
            f.write("\n")
            f.write(body22)
            f.write(alias)
        print("[gen_fonts] 已生成真实中文字体 ->", OUT)
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        print("[gen_fonts] lv_font_conv 失败, 保留 main/ui_fonts.c 英文占位字体:", e)
    finally:
        for tmp in (GEN16, GEN22):
            if os.path.exists(tmp):
                os.remove(tmp)


if __name__ == "__main__":
    main()
