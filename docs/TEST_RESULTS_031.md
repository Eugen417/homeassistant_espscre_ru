# Rectangular tiles, 0.3.1 acceptance

Tested on 24 September 2026, on top of the page-owned layout refactor. The scope is two additional footprints, 1 by 2 and 2 by 2 cells, with existing card designs. The grid, number of cells and page capacity stay fixed. Full-page presentation remains a separate choice.

## Contracts and editor

`tools/check.sh` passes 697 Python tests, 25 C++ programs, the editor tests and type checks, and the generated bundle, package, board, icon and translation checks.

Automated coverage checks rectangular occupancy, overlap refusal, page boundaries, one-column boards, packing order, stable document round trips, grid adaptation, resizing beside occupied cells, and capability negotiation. A 2 by 2 card retains wide-card controls; a 1 by 2 card retains single-card behavior. Forecast and sun-path views still require two columns.

The real Home Assistant OS test add-on was upgraded to 0.3.1 with its existing layout intact. With the physical screen still on 0.3.0, it remained synchronized and the new sizes were absent from the editor. Updating the screen exposed both sizes automatically in the already-open inspector. The sender refuses unsupported sizes before starting a configuration transaction.

Both sizes were selected and saved through the browser and actual Home Assistant Ingress. The board acknowledged the saved revision. A tall tile was dragged into a free two-row rectangle and Undo restored it without moving its neighbor. A drop that would cross the bottom of a page left the layout unchanged.

No storage version change or migration was added. Width and height use the existing page-document placement. The firmware advertises its supported presentations and renders one configuration, without a second compatibility layout.

## Physical board

An explicitly selected disposable Guition 4848S040 was flashed over USB and its compilation identity read back. A four-page example uses demo Home Assistant entities: a tall light with a brightness slider, square media and climate cards, and tall graph and analog-clock cards. Snapshots of the real LVGL render were inspected. Ordinary neighboring tiles retain their size.

The board's self-test passed ten page geometry checks and fifty overlay render cycles with the seven-tile example. This checks rendering on the physical board, not finger feel or the GT911 sensor. No physical taps were performed for this extension and no household device actions were used.

## Responsiveness regression

The native host tests add tall light, graph and clock cards plus square media, climate and clock cards, with ordinary neighbors in uncovered cells. They check actual LVGL placement, dimensions and overlap. One-column profiles exercise only the tall footprint.

All ten host variants passed across the completed runs, totaling 380 page checks. Initial runs had intermittent missing simulated swipes on three variants; separate reruns passed the navigation assertions and rectangle checks. This remains a limitation of timing-sensitive host input tests, not evidence of physical touchscreen acceptance.

This exposed a clock edge case on a large portrait layout: the dial left no room for the adjacent digital labels. The renderer now applies its existing no-room behavior to wide multi-row clocks too, hiding the adjacent text and centering the dial. The fixture remains a regression check.

## Firmware builds

All six boards compile with ESPHome 2026.9.0 after the clock correction.

| Board | Firmware size |
| --- | ---: |
| CYD | 1,632,544 B |
| Guition | 2,050,000 B |
| Waveshare 4.3 | 2,306,432 B |
| JC8012P4A1 | 2,022,736 B |
| Waveshare 7 | 1,835,440 B |
| Waveshare 4B | 2,035,504 B |

The CYD retains 202,464 bytes in its OTA slot. Growth against the matching 0.3.0 build is 1,424 bytes. These are flash measurements; the long memory workloads documented for 0.3.0 were not repeated for this extension.
