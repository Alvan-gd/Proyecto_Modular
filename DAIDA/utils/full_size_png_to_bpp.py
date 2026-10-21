from PIL import Image
from pathlib import Path


def convert_png_to_1bpp(input_file, output_file):
    # Open image
    image = Image.open(input_file)

    # Ensure exactly 240x280
    if image.size != (240, 280):
        raise ValueError(
            f"Image must be 240x280, but got {image.size[0]}x{image.size[1]}"
        )

    # Convert to grayscale
    image = image.convert("L")

    pixels = image.load()

    data = []

    # Process each row
    for y in range(280):
        current_byte = 0

        for x in range(240):
            # Black = 1, white = 0
            if pixels[x, y] < 128:
                current_byte |= (1 << (7 - (x % 8)))

            # Every 8 pixels form one byte
            if x % 8 == 7:
                data.append(current_byte)
                current_byte = 0

    # Fixed array name
    array_name = "daida_bitmap"

    with open(output_file, "w") as f:
        f.write("#pragma once\n")
        f.write("#include <stdint.h>\n\n")

        f.write("#define SPLASH_WIDTH 240\n")
        f.write("#define SPLASH_HEIGHT 280\n\n")

        # 240 * 280 / 8 = 8400 bytes
        f.write(
            f"static const uint8_t {array_name}[8400] = {{\n"
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
    icons_dir = Path("components/lcd_st7789/icons")

    for file in icons_dir.glob("*.png"):
        # Solo procesamos la imagen destinada al splash screen (240x280)
        try:
            with Image.open(file) as img:
                if img.size == (240, 280):
                    output_file = icons_dir / "daida_bitmap.h"
                    convert_png_to_1bpp(str(file), str(output_file))
                    print(f"Splash bitmap generado con éxito: {output_file.name}")
        except Exception as e:
            print(f"Error procesando {file.name}: {e}")