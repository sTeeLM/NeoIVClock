import argparse
import os
import sys
import re
from PIL import Image, ImageFont, ImageDraw
from pypinyin import pinyin, Style

def is_chinese(ch):
    """判断一个字符是否是汉字"""
    return '\u4e00' <= ch <= '\u9fff'

def get_pinyin_var_name(char, used_names):
    """根据汉字生成不重复的拼音变量名（全大写）"""
    py_list = pinyin(char, style=Style.NORMAL)
    if py_list and py_list[0]:
        base_name = f"mini_font_chinese_{py_list[0][0].upper()}"
    else:
        base_name = f"mini_font_chinese_{ord(char):04X}"
    
    base_name = re.sub(r'[^A-Za-z0-9_]', '', base_name)
    
    if base_name not in used_names:
        used_names[base_name] = 0
        return base_name
    else:
        counter = 1
        while True:
            test_name = f"{base_name}{counter}"
            if test_name not in used_names:
                used_names[test_name] = 0
                return test_name
            counter += 1

def generate_ssd1306_bitmap(char, width, height, font_path):
    """利用 Pillow 绘制汉字并按照 SSD1306 页格式（纵向扫描）提取点阵"""
    img = Image.new("1", (width, height), 0)
    draw = ImageDraw.Draw(img)
    
    try:
        font = ImageFont.truetype(font_path, min(width, height))
    except IOError:
        print(f"Error: 无法加载字体文件: {font_path}", file=sys.stderr)
        sys.exit(1)

    left, top, right, bottom = draw.textbbox((0, 0), char, font=font)
    w_text = right - left
    h_text = bottom - top
    x_offset = (width - w_text) // 2 - left
    y_offset = (height - h_text) // 2 - top
    
    draw.text((x_offset, y_offset), char, font=font, fill=1)
    
    aligned_height = (height + 7) & ~7
    pages = aligned_height // 8
    bitmap_bytes = []
    
    for p in range(pages):
        for x in range(width):
            byte_val = 0
            for bit in range(8):
                y = p * 8 + bit
                if y < height:
                    pixel = img.getpixel((x, y))
                    if pixel > 0:
                        byte_val |= (1 << bit)
            bitmap_bytes.append(byte_val)
            
    return bitmap_bytes

def main():
    parser = argparse.ArgumentParser(
        description="从文本文件读取汉字并转换为SSD1306 OLED C语言点阵字库工具"
    )
    parser.add_argument("-i", "--input", required=True, help="输入的汉字文本文件路径 (UTF-8 编码)")
    parser.add_argument("-w", "--width", type=int, required=True, help="字模宽度 (如 16)")
    parser.add_argument("-g", "--height", type=int, required=True, help="字模高度 (如 16)")
    parser.add_argument("-f", "--font", required=True, help="TTF字体文件路径")
    parser.add_argument("-t", "--type_macro", default="MINI_FONT_TYPE_CHINESE_16X16", help="C结构体中的类型宏定义名称")
    
    # 【检查1】核心改动：如果没有任何命令行参数，直接打印帮助信息并退出
    if len(sys.argv) == 1:
        parser.print_help(sys.stderr)
        print("\n[错误] 未检测到任何命令行参数，程序已退出。", file=sys.stderr)
        sys.exit(1)
        
    args = parser.parse_args()
    
    # 【检查2】验证输入的汉字文本文件是否存在
    if not os.path.isfile(args.input):
        print(f"[错误] 输入的汉字文本文件不存在: {args.input}", file=sys.stderr)
        sys.exit(1)
        
    # 【检查3】验证字体文件是否存在
    if not os.path.isfile(args.font):
        print(f"[错误] 指定的字体文件不存在: {args.font}", file=sys.stderr)
        sys.exit(1)

    # 【检查4】检查宽高是否为正整数
    if args.width <= 0 or args.height <= 0:
        print(f"[错误] 字模的宽度(-w)与高度(-g)必须为大于 0 的正整数！当前输入为: {args.width}x{args.height}", file=sys.stderr)
        sys.exit(1)

    # 开始处理文件
    unique_chars = []
    with open(args.input, "r", encoding="utf-8") as f:
        content = f.read()
        for ch in content:
            if is_chinese(ch) and ch not in unique_chars:
                unique_chars.append(ch)

    if not unique_chars:
        print("[警告] 未在输入文件中检索到任何有效的汉字字符。", file=sys.stderr)
        sys.exit(0)

    used_names = {}
    char_nodes = []

    print("/* ==================== 汉字点阵数组定义开始 ==================== */\n")

    for char in unique_chars:
        var_name = get_pinyin_var_name(char, used_names)
        bytes_data = generate_ssd1306_bitmap(char, args.width, args.height, args.font)
        
        if bytes_data is None:
            continue
            
        char_nodes.append((char, var_name))
        
        print(f"static const uint8_t {var_name}[] =\n{{")
        for i in range(0, len(bytes_data), 16):
            chunk = bytes_data[i:i+16]
            hex_str = ",".join([f"0x{b:02X}" for b in chunk])
            if i + 16 < len(bytes_data):
                hex_str += ","
            print(f"    {hex_str}")
        print(f"    /*{char} ({args.width} X {args.height} )*/")
        print("};\n")

    print("/* ==================== 汉字点阵数组定义结束 ==================== */\n")

    print(f"mini_font_node_t mini_font_chinese_font_{args.width}x{args.height}[] =\n{{")
    for char, var_name in char_nodes:
        print(f"    {{L'{char}', {var_name}, {args.type_macro}, NULL}},")
    print("};")

if __name__ == "__main__":
    main()

