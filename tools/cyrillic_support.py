#!/usr/bin/env python3
"""Support for Cyrillic characters in font generation."""

import re
from pathlib import Path


def get_cyrillic_glyphs():
    """Get list of Cyrillic glyphs for font generation"""
    # Cyrillic characters (Unicode range 0x0400-0x04FF)
    cyrillic_chars = [
        # Capital letters
        0x0400, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407,
        0x0408, 0x0409, 0x040A, 0x040B, 0x040C, 0x040D, 0x040E, 0x040F,
        # Cyrillic capital letters (A-Z)
        0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
        0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
        0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
        0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
        # Small letters
        0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
        0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
        0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
        0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
        # Special characters
        0x0451, 0x0452, 0x0453, 0x0454, 0x0455, 0x0456, 0x0457, 0x0458,
        0x0459, 0x045A, 0x045B, 0x045C, 0x045D, 0x045E, 0x045F
    ]
    
    # Convert to string format for glyph list
    glyphs = [f'      - "\\U000{code:04x}" # Cyrillic character {hex(code)}' for code in cyrillic_chars]
    return '\n'.join(glyphs)


def detect_language_from_font(font_name):
    """Detect if font should support Cyrillic based on language"""
    # Simple detection - if font name contains 'ru', 'cyrl', or other Cyrillic indicators
    return 'ru' in font_name.lower() or 'cyrl' in font_name.lower()


def add_cyrillic_support_to_profile(profile_content, font_name):
    """Add Cyrillic support to a font profile"""
    if detect_language_from_font(font_name):
        cyrillic_glyphs = get_cyrillic_glyphs()
        
        # Find the glyphs section and add Cyrillic characters
        pattern = r'(    glyphs:.*?)(\n    )'
        replacement = rf'\1\n{cyrillic_glyphs}\2'
        
        # Add cyrillic support to the profile content
        modified_content = re.sub(pattern, replacement, profile_content, flags=re.DOTALL)
        return modified_content
    
    return profile_content


if __name__ == "__main__":
    print("Cyrillic support module loaded")