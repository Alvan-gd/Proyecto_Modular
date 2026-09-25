from PIL import Image
import sys
from pathlib import Path


def convert_png_to_1bpp(input_file, output_file):
    # Open image
    image = Image.open(input_file)

    # Ensure exactly 64x64
    if image.size != (64, 64):
        raise ValueError(
            f"Image must be 64x64, but got {image.size[0]}x{image.size[1]}"
        )

    # Convert to grayscale
    image = image.convert("L")

    pixels = image.load()

    data = []

    # Process each row
    for y in range(64):
        current_byte = 0

        for x in range(64):
            # Black = 1, white = 0
            if pixels[x, y] < 128:
                current_byte |= (1 << (7 - (x % 8)))

            # Every 8 pixels form one byte
            if x % 8 == 7:
                data.append(current_byte)
                current_byte = 0

    # Generate C header
    array_name = Path(input_file).stem + "_bitmap"

    with open(output_file, "w") as f:
        f.write("#pragma once\n")
        f.write("#include <stdint.h>\n\n")

        f.write("#define ICON_WIDTH 64\n")
        f.write("#define ICON_HEIGHT 64\n\n")

        f.write(
            f"static const uint8_t {array_name}[512] = {{\n"
        )

        for i, byte in enumerate(data):
            if i % 16 == 0:
                f.write("    ")

            f.write(f"0x{byte:02X}")

            if i != len(data) - 1:
                f.write(", ")

            if i % 16 == 15:
                f.write("\n")

        f.write("};\n")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage:")
        print("python png_to_1bpp.py input.png output.h")
        sys.exit(1)

    convert_png_to_1bpp(sys.argv[1], sys.argv[2])

    print(f"Converted {sys.argv[1]} -> {sys.argv[2]}")