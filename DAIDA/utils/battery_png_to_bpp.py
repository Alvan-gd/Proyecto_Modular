from PIL import Image
import sys
from pathlib import Path


def convert_png_to_1bpp(input_file, output_file):
    # Open image
    image = Image.open(input_file)

    # Ensure exactly 32x24
    if image.size != (32, 24):
        raise ValueError(
            f"Image must be 32x24, but got {image.size[0]}x{image.size[1]}"
        )

    # Convert to grayscale
    image = image.convert("L")

    pixels = image.load()

    data = []

    # Process each row (Height = 24 rows)
    for y in range(24):
        current_byte = 0

        # Process each column (Width = 32 pixels)
        for x in range(32):
            # Black = 1, white = 0
            if pixels[x, y] < 128:  # Se mantiene umbral estándar de 128
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

        f.write("#define BATTERY_ICON_WIDTH 32\n")
        f.write("#define BATTERY_ICON_HEIGHT 24\n\n")

        # 32 * 24 / 8 = 96 bytes
        f.write(
            f"static const uint8_t {array_name}[96] = {{\n"
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
    icons_dir = Path("components/lcd_st7789/battery_icons")

    for file in icons_dir.glob("*.png"):
        output_file = file.with_name(f"{file.stem}.h")
        try:
            convert_png_to_1bpp(str(file), str(output_file))
            print(f"Procesado: {file.name}")
        except Exception as e:
            print(f"Error procesando {file.name}: {e}")