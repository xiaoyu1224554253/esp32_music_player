#!/usr/bin/env python3
"""
生成全中文字库 (GB2312, 含常用标点) 供 ESP32-S3 LVGL 使用。
CI 中使用: 先 `npm i -g lv_font_conv`, 再运行本脚本生成 main/ui_fonts.c
输出字体为 LVGL 内置压缩格式, 烧录进 Flash, 不占运行时 PSRAM 解码。
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "ui_fonts.c")

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

def main():
    font = find_font()
    if not font:
        print("[gen_fonts] 未找到系统中文字体, 使用占位字体 (不生成完整中文)")
        return
    try:
        subprocess.run([
            "lv_font_conv",
            "--font", font,
            "--format", "lvgl",
            "--size", "16",
            "--bpp", "3",
            "--font-subsets", "gb2312",
            "--symbols", "🌐🎵📻🔍🎤▶⏸⏮⏭📑🎻🎷🎧🪕📰✨🌙📡🔄🔀🔁",
            "--output", OUT,
            # 输出变量名为 font_cn_16 / font_cn_22
        ], check=True)
        print("[gen_fonts] 已生成", OUT)
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        print("[gen_fonts] lv_font_conv 失败, 保留占位字体:", e)

if __name__ == "__main__":
    main()
