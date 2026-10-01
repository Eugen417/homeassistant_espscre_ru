# A map card

App 0.4.33 with firmware 0.20.0 can turn a person tile into a map: the streets around them, the zones you have in Home
Assistant, and a marker for everyone on the card. ESP Screens draws the whole card in the screen's own colours and sends
the screen a picture, the way a live camera arrives. The screen never gets a location.

## Setting it up

1. Put a `person.*` entity on a page and set **Display** to **Map**.
2. **Show** picks what the card frames:
   - **Everyone**: the smallest view that holds everyone on the card, with the zone each of them is in.
   - **Around home**: your home zone in the middle.
   - **Around this person**: the tile's own person in the middle.
3. With Around home or Around this person, **Distance** says how far the view reaches: Street, Neighborhood, Town or
   Region. Someone outside the view is a small marker on the edge of the card, pointing the way to them.
4. **Also on the map** adds other people and device trackers, up to eight on one card (app 0.4.35 for trackers). A
   device tracker is anything Home Assistant reports a place for: a phone through the Companion app, a car through its
   integration, a tag. One that only knows home or away has no place and is not offered. Each keeps a colour, in the
   order they are listed, and their initials are in their marker. From a card about 200 pixels high the first names are beside the
   markers.
5. **On the picture** says whether the tile's name is on the map, on a small label at the bottom left.

A map works on every size, from one cell to the whole page, on every board that draws pictures (the boards with
`camera` in `screen_manager/app/boards.json`). The CYD and the other boards without memory for pictures do not offer it,
and a layout with one is refused on them.

## Where the streets come from

Home Assistant Core has a `map_tiles` integration, which its own map card uses. It fetches OpenStreetMap's vector tiles
with Home Assistant's own identification, as the OpenStreetMap tile policy asks, and keeps them for a week. ESP Screens
asks Home Assistant for those tiles and never contacts a tile server itself. A request names a zoom and two whole numbers:
no entity, no name.

A vector tile says what is there (a street of some kind, water, a park) and nothing about how it looks, so ESP Screens
draws it in the screen's colours: a quiet ground of greys with white streets, water and green in soft tints, in light and
dark. The only strong colours on the card are the people and their zones. The card carries "© OpenStreetMap" at the
top right.

When Home Assistant has no tiles to give (an older Home Assistant, or no internet), the card is drawn from the zones and
the people alone, and ESP Screens asks again after ten minutes.

## When it is drawn again

Never on a clock. With the layout, each map tile gets a short movement mark: a hash of where its people are (rounded to
about 25 meters, so a phone's drift is no change), their states, the zones, and the tile's own choices and name. The
screen asks for a new picture when the mark changes. A household that stays put costs nothing.

## Light and dark

The screen says which look it is in when it asks for its pictures, and gets the map drawn for that look. A screen in
dark mode and one in light mode showing the same card each get their own picture. ESP Screens keeps the last maps it drew
by their mark, their size and their look, so a page that loads again for a camera next to it does not draw its map again.
The screen keeps its pictures under a name that includes the look as well, so switching the look asks for the other map.

## Privacy

- **No location reaches a screen.** Who is on a card and how it frames them stay in ESP Screens; the screen gets the
  movement mark and pixels.
- **The picture travels unencrypted over your network**, on port 8098, like every other picture. A map shows roughly
  where someone is to anyone who can read that traffic. Keep that in mind for a screen on a guest network.
- A screen only gets a map for a map tile on its own saved layout, drawn from that saved tile.

## For developers

| Piece | Where |
| --- | --- |
| Reading Home Assistant's vector tiles | `screen_manager/app/vector_tiles.py` |
| The tiles through Home Assistant, cached | `screen_manager/app/map_tiles.py`, `HomeAssistant.map_tile` in `server.py` |
| Framing, the movement mark and the drawing | `screen_manager/app/map_card.py` |
| A screen's request, the kept maps | `Manager.answer_live` and `Manager.map_render` in `server.py`, `CameraFeed.live` |
| The choices | `catalogue/person.yaml` (`map`), `core.validate_layout` |
| The firmware | `Tile::is_map()`, `card_art`, `live_wanted` (the mark and `dark`) in `components/smart_display` |
| The editor | `TileInspector.vue`, `model/tile-options.ts`, `model/page-validation.ts` |
| Tests and renders | `tests/test_map_card.py`, `tools/render/map_tiles.py` |

The picture pipeline the map shares is described in [CAMERA.md](CAMERA.md).
