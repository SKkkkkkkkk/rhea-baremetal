#!/usr/bin/env python3
import argparse
import struct
import sys

def parse_args():
    def positive_int(value):
        ivalue = int(value)
        if ivalue <= 0:
            raise argparse.ArgumentTypeError(f"{value} is not a positive integer.")
        return ivalue

    parser = argparse.ArgumentParser(
        description="Convert binary file to ASCII hex.",
        epilog=(
            "Example usage: python bin2hex.py input.bin --unit-size 4 --line-size 2 --byte-order little --unit-order big\n"
        )
    )
    parser.add_argument("input_file", help="Input binary file to be converted.")
    parser.add_argument("--unit-size", type=positive_int, default=4, help="Unit size in bytes (default: 4). Must be a positive integer.")
    parser.add_argument("--line-size", type=positive_int, default=1, help="Number of units per line (default: 1). Must be a positive integer.")
    parser.add_argument("--byte-order", choices=["little", "big"], default="little", help="Byte order of a unit (default: little).")
    parser.add_argument("--unit-order", choices=["little", "big"], default="little", help="Unit order of a line (default: little).")
    return parser.parse_args()

def read_binary_file(file_path):
    """Read the binary file and return its content."""
    with open(file_path, "rb") as f:
        return f.read()

def pad_data(data, unit_size, line_size):
    """Pad the data to ensure it aligns with the line size."""
    line_byte_size = unit_size * line_size
    padding_needed = (line_byte_size - len(data) % line_byte_size) % line_byte_size
    return data + b'\x00' * padding_needed

def convert_to_hex(data, unit_size, line_size, byte_order, unit_order):
    """Convert binary data to hex representation."""
    hex_lines = []
    line_byte_size = unit_size * line_size
    struct_format = f"{byte_order[0]}{unit_size}s"

    for i in range(0, len(data), line_byte_size):
        line_data = data[i:i + line_byte_size]
        units = [line_data[j:j + unit_size] for j in range(0, len(line_data), unit_size)]
        
        if byte_order == "little":
            units = [unit[::-1] for unit in units]
        if unit_order == "little":
            units.reverse()
        
        hex_line = ''.join(unit.hex() for unit in units)
        hex_lines.append(hex_line)
    
    return hex_lines

def main():
    args = parse_args()
    data = read_binary_file(args.input_file)
    padded_data = pad_data(data, args.unit_size, args.line_size)
    hex_lines = convert_to_hex(padded_data, args.unit_size, args.line_size, args.byte_order, args.unit_order)
    
    try:
        for line in hex_lines:
            print(line)
    except BrokenPipeError:
        sys.stderr.close()

if __name__ == "__main__":
    main()