from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ADDON = Path(__file__).resolve().parents[1]
PROJECT = Path(__file__).resolve().parents[3]
SOURCE_ATLAS = PROJECT / "gamedata" / "textures" / "ui" / "pf_qa_tools_icons.dds"
OUTPUT_DDS = ADDON / "textures" / "ui" / "pf_qa_point_logger_icon.dds"
OUTPUT_PREVIEW = ADDON / "tools" / "pf_qa_point_logger_icon_preview.png"
FONT = Path("C:/Windows/Fonts/arialbd.ttf")
SOURCE_TILE_INDEX = 13
TILE_SIZE = 50
RENDER_SCALE = 4
PINK = (255, 92, 184)
LABEL = ("ЛОГ", "ТОЧЕК")


def recolor_blue_accent(tile: Image.Image) -> Image.Image:
    source = tile.convert("RGBA")
    result = source.copy()
    source_pixels = source.load()
    result_pixels = result.load()
    for y in range(source.height):
        for x in range(source.width):
            red, green, blue, alpha = source_pixels[x, y]
            if alpha and blue > red + 18 and blue >= green:
                strength = max(red, green, blue) / 255.0
                result_pixels[x, y] = (
                    round(PINK[0] * strength),
                    round(PINK[1] * strength),
                    round(PINK[2] * strength),
                    alpha,
                )
    return result


def fit_font(draw: ImageDraw.ImageDraw) -> ImageFont.FreeTypeFont:
    max_width = 42 * RENDER_SCALE
    max_height = 29 * RENDER_SCALE
    label = "\n".join(LABEL)
    for size in range(13 * RENDER_SCALE, 6 * RENDER_SCALE, -1):
        font = ImageFont.truetype(str(FONT), size)
        box = draw.multiline_textbbox(
            (0, 0), label, font=font, spacing=-RENDER_SCALE,
            align="center", stroke_width=RENDER_SCALE,
        )
        if box[2] - box[0] <= max_width and box[3] - box[1] <= max_height:
            return font
    raise RuntimeError("Unable to fit the point-logger icon label")


def build_tile() -> Image.Image:
    atlas = Image.open(SOURCE_ATLAS).convert("RGBA")
    left = SOURCE_TILE_INDEX * TILE_SIZE
    tile = atlas.crop((left, 0, left + TILE_SIZE, TILE_SIZE))
    tile = recolor_blue_accent(tile)
    render_size = TILE_SIZE * RENDER_SCALE
    tile = tile.resize((render_size, render_size), Image.Resampling.LANCZOS)
    draw = ImageDraw.Draw(tile)
    draw.rounded_rectangle(
        (3 * RENDER_SCALE, 6 * RENDER_SCALE, 47 * RENDER_SCALE, 44 * RENDER_SCALE),
        radius=3 * RENDER_SCALE,
        fill=(3, 5, 5, 235),
        outline=(*PINK, 210),
        width=RENDER_SCALE,
    )
    label = "\n".join(LABEL)
    font = fit_font(draw)
    box = draw.multiline_textbbox(
        (0, 0), label, font=font, spacing=-RENDER_SCALE,
        align="center", stroke_width=RENDER_SCALE,
    )
    x = (render_size - (box[2] - box[0])) // 2 - box[0]
    y = (render_size - (box[3] - box[1])) // 2 - box[1]
    draw.multiline_text(
        (x, y), label, font=font, fill=(*PINK, 255),
        spacing=-RENDER_SCALE, align="center",
        stroke_width=RENDER_SCALE, stroke_fill=(0, 0, 0, 255),
    )
    return tile.resize((TILE_SIZE, TILE_SIZE), Image.Resampling.LANCZOS)


def main() -> None:
    tile = build_tile()
    output = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    output.alpha_composite(tile, (0, 0))
    OUTPUT_DDS.parent.mkdir(parents=True, exist_ok=True)
    output.save(OUTPUT_DDS, format="DDS", pixel_format="DXT5")

    OUTPUT_PREVIEW.parent.mkdir(parents=True, exist_ok=True)
    Image.open(OUTPUT_DDS).convert("RGBA").crop((0, 0, 50, 50)).save(OUTPUT_PREVIEW)


if __name__ == "__main__":
    main()
