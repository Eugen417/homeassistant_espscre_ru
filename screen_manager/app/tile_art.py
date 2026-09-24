"""Prepare bounded rectangular tile artwork in the add-on, never on the board.

An atlas reuses the screen's existing live-image buffer. Frames use canvas
coordinates, so mixed tile footprints and repeated entities need no extra bitmap
per tile. The allocation is bounded by the paired screen's reported canvas.
"""
import io
import json


def parse(value, canvas, count):
    """Validated immutable (width, height, frames), or None for malformed input.

    Each frame is x, y, width, height, corner radius, darkening (0 or 170).
    The last two fields are baked into RGB pixels, avoiding device mask layers.
    """
    if not isinstance(value, str) or len(value) > 8192 or not 1 <= count <= 64:
        return None
    try:
        frames = json.loads(value)
    except (ValueError, TypeError):
        return None
    if not isinstance(frames, list) or len(frames) != count:
        return None
    width, height = canvas
    if width <= 0 or height <= 0:
        return None
    checked = []
    right = bottom = 0
    for frame in frames:
        if not isinstance(frame, list) or len(frame) != 6 or any(type(n) is not int for n in frame):
            return None
        x, y, w, h, radius, shade = frame
        if x < 0 or y < 0 or w < 1 or h < 1 or x+w > width or y+h > height:
            return None
        if not 0 <= radius <= min(w, h)//2 or shade not in (0, 170):
            return None
        if any(x < a+c and a < x+w and y < b+d and b < y+h for a,b,c,d,_,_ in checked):
            return None
        checked.append(tuple(frame))
        right, bottom = max(right, x+w), max(bottom, y+h)
    return right, bottom, tuple(checked)


def encode(raws, grounds, atlas):
    """A native-sized BMP, cropped, dimmed and rounded before transmission."""
    from PIL import Image, ImageDraw, ImageOps
    width, height, frames = atlas
    image = Image.new('RGB', (width, height))
    for raw, background, frame in zip(raws, grounds, frames):
        x, y, w, h, radius, shade = frame
        colour = tuple((background >> shift) & 255 for shift in (16, 8, 0))
        tile = Image.new('RGB', (w, h), colour)
        if raw is not None:
            with Image.open(io.BytesIO(raw)) as source:
                source.draft('RGB', (w*2, h*2))
                source = ImageOps.exif_transpose(source)
                # Transparent source pixels use the tile's ground, not black.
                rgba = source.convert('RGBA')
                opaque = Image.new('RGBA', rgba.size, colour + (255,))
                opaque.alpha_composite(rgba)
                cover = ImageOps.fit(opaque.convert('RGB'), (w, h), method=Image.Resampling.LANCZOS)
            if shade:
                cover = Image.blend(cover, Image.new('RGB', cover.size), shade/255)
            mask = Image.new('L', (w*4, h*4))
            ImageDraw.Draw(mask).rounded_rectangle((0, 0, w*4-1, h*4-1), radius=radius*4, fill=255)
            mask = mask.resize((w, h), Image.Resampling.LANCZOS)
            tile.paste(cover, mask=mask)
        image.paste(tile, (x, y))
    output = io.BytesIO()
    image.save(output, 'BMP')
    return output.getvalue()
