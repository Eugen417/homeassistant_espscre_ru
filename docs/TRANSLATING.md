# Translating ESP Screens

Every text of ESP Screens lives in one file per language: `screen_manager/translations/<code>.json`. The screens, the
app and the editor all read the same file. English (`en.json`) is the source: every other file has the same keys, and a
key a language doesn't have yet shows in English.

The code is the one Home Assistant uses for the language: `nl`, `de`, `fr`, `pt-BR`, `zh-Hans`. A screen takes Home
Assistant's language unless someone picks another one under Settings in ESP Screens; the editor follows the language of
your own Home Assistant profile.

## The sections

| Section | Where it shows | Keep in mind |
| --- | --- | --- |
| `screen` | On the screen itself: built into its firmware when it is installed or updated | Small screens: keep it as short as the English. One language per screen, so this costs almost no room. |
| `addon` | Messages of the ESP Screens app, and the texts it sends to the screens | Error messages are whole sentences. |
| `editor` | The ESP Screens editor in Home Assistant | |
| `_meta` | The language itself | `name` in the language itself, `english` in English, `plural` (below), `checked` |

`screen.ha` holds Home Assistant's own words (on, off, open, heating, ...). They come from Home Assistant's translations
through `tools/i18n.py`; don't change them by hand, change them in Home Assistant's translations instead.
`screen.date` holds day and month names and date orders from the Unicode CLDR, the same data browsers use.

## The format

- **Placeholders** such as `{n}` or `{name}` stay exactly as they are; move them where the sentence needs them.
- **Plurals** are the forms of a sentence separated by ` | `, in the order of the file's `_meta.plural` rule:

  | Rule | Languages | Forms |
  | --- | --- | --- |
  | `one_other` | English, Dutch, German, Italian, Spanish, the Nordic languages | `1 hour ago \| {n} hours ago` |
  | `one_upto_1` | French, Portuguese | the first form for 0 and 1 |
  | `slavic_pl` | Polish | one \| few (2-4, 22-24, ...) \| many |
  | `east_slavic` | Russian, Ukrainian | one \| few \| many |
  | `none` | Chinese, Japanese, Korean | one form |

- Don't use `{`, `}`, `|` or `@` as ordinary characters.
- `_meta.checked` is `true` once someone who speaks the language has checked the whole file. The settings in ESP Screens
  show a language that isn't checked yet with a request for help.

## Letters the screens can draw

The screens carry every letter European languages write with the Latin alphabet. A language in another script
(Cyrillic, Greek, Chinese) gets its letters added only on the screens that use it, and only where they fit: a CYD has
too little room for Chinese and shows English then. `tools/i18n.py check` says when a text uses a letter the screens
can't draw.

## Checking your work

```bash
python3 tools/i18n.py check
```

It lists missing keys, placeholders that differ from English, the wrong number of plural forms, letters the screens
can't draw, and screen texts that are much longer than the English.
