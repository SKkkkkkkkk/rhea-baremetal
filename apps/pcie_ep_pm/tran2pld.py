import sys
def reorganize_data(input_file, output_file):#{{{
    with open(input_file, 'r') as f:
        # 从hex文件读取数据并去除换行符
        input_data = [line.strip() for line in f.readlines()]
    remaining_lines = 16 - (len(input_data) % 16)
    if remaining_lines != 16:
        input_data.extend(['00000000'] * remaining_lines)
    grouped_data = [input_data[i:i+4] for i in range(0, len(input_data), 4)]

    result = ""
    for group in grouped_data:
        result += f"{group[3]}{group[2]}{group[1]}{group[0]}\n"

    # 将结果写入新的hex文件
    with open(output_file, 'w') as f:
        f.write(result)#}}}

# 计算需要添加的空行数，确保行数��

if __name__ == '__main__':
    reorganize_data(sys.argv[1],"build/boot_rom.hex")