import os
import sys
import math
from PIL import Image

def bmp_to_font_bitmap_array(image_path):
    """
    根据实际图片大小，转换为符合字体取模规范的字节数组
    格式: 纵向扫描, LSB First (低位在上), 列从左到右, 页从上到下
    """
    try:
        img = Image.open(image_path)
    except Exception as e:
        print(f"错误: 无法打开文件 '{image_path}' ({e})", file=sys.stderr)
        return None, 0, 0
        
    # 强制转换为单色位图
    img = img.convert('1')
    width, height = img.size
    
    # 计算实际需要的页数（向上取整，确保高度不是8倍数时也能处理）
    pages = math.ceil(height / 8)
    
    bytes_list = []
    
    # 以 8 像素高的“页(Page)”为单位循环
    for page in range(pages):
        y_start = page * 8
        # 列从左到右扫描
        for x in range(width):
            current_byte = 0
            # 纵向 8 个像素拼成 1 个字节
            for bit in range(8):
                y = y_start + bit
                
                # 如果超过了图片实际高度，用白色像素(背景)填充
                if y < height:
                    pixel = img.getpixel((x, y))
                else:
                    pixel = 255 # 1-bit 图中 255 通常代表白色背景
                
                # 默认：原图中的黑色像素(0)在屏幕上点亮(1)
                # 如果屏幕显示依旧反色，请把 == 0 改为 > 0
                if pixel == 0:
                    current_byte |= (1 << bit)
                    
            bytes_list.append(f"0x{current_byte:02X}")
                
    return bytes_list, width, height

def main():
    # 检查参数数量是否正确
    if len(sys.argv) != 2:
        print("使用方法: python bmp_to_stdout_single.py <文件名.bmp>", file=sys.stderr)
        sys.exit(1)

    bmp_path = sys.argv[1]

    # 检查文件是否存在
    if not os.path.exists(bmp_path):
        print(f"错误: 文件 '{bmp_path}' 不存在", file=sys.stderr)
        sys.exit(1)

    # 提取文件名并清洗为合法的 C 语言变量名
    base_name = os.path.splitext(os.path.basename(bmp_path))[0]
    var_name = "".join([c if c.isalnum() else "_" for c in base_name])
    var_name = f"icon_{var_name}"
    
    hex_data, width, height = bmp_to_font_bitmap_array(bmp_path)
    if hex_data is None:
        sys.exit(1)
        
    # 格式化打印数组到 stdout，注释中标注实际宽高
    print(f"// 源文件: {bmp_path}")
    print(f"// 图像尺寸: {width} x {height}")
    print(f"const unsigned char {var_name}[] PROGMEM = {{")
    
    # 每行打印 16 个字节
    for i in range(0, len(hex_data), 16):
        row_bytes = hex_data[i:i+16]
        print("    " + ", ".join(row_bytes) + ",")
        
    print("};")

if __name__ == "__main__":
    main()

