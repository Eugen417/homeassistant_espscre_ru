"""History on a screen's detail card (app 0.2.59 / firmware 0.2.51).

A card asks for it when it opens or when its range changes: the screen fires `esphome.screen_history` with its
inbox, the entity and the hours (1, 24 or 168), and the manager answers with one `history` message. Numbers get a
line of 24 points, averaged over equal parts of the range, with their highest and lowest moment and an axis in
round steps; states (on/off, home/away, a status sensor) get a timeline of at most 96 slots with the time spent
in each state, as Home Assistant's own history does. The screen draws it and formats the times itself, so its
12- or 24-hour clock applies.
"""
import math
from datetime import datetime, timedelta, timezone

import header_bar
from core import state_word

RANGES = (1, 24, 168)
POINTS = 24
SLOTS = 96
STATES_SHOWN = 6
# Entities whose card shows history, by what they show.
LINE_DOMAINS = frozenset(('number', 'input_number'))
TIMELINE_DOMAINS = frozenset(('binary_sensor', 'switch', 'input_boolean', 'person'))
# Sensors whose state is not a measurement.
TEXT_SENSOR_CLASSES = frozenset(('timestamp', 'date', 'enum'))
# The state that counts as "on" for the count of times it began.
ACTIVE = {'binary_sensor': 'on', 'switch': 'on', 'input_boolean': 'on', 'person': 'home'}
OFF_GREY, NO_DATA = 'CFCFCF', 'E6E6E6'
PALETTE = ('2196F3', '009688', 'FF6F22', '8E24AA', '43A047', 'E53935')
# How long an answer stays good: a card reopened within this time gets it without asking Home Assistant again.
CACHE_SECONDS = {1: 30, 24: 120, 168: 600}


def kind(entity, state):
    """'line', 'timeline', or None for an entity whose card has no history."""
    domain = entity.split('.')[0]
    if domain in LINE_DOMAINS:
        return 'line'
    if domain in TIMELINE_DOMAINS:
        return 'timeline'
    if domain != 'sensor':
        return None
    attrs = (state or {}).get('attributes') or {}
    if attrs.get('device_class') in TEXT_SENSOR_CLASSES:
        return 'timeline'
    if attrs.get('unit_of_measurement') or attrs.get('state_class'):
        return 'line'
    return 'line' if header_bar.numeric((state or {}).get('state')) is not None else 'timeline'


def window(hours, now=None):
    """(start, end) unix times of the range that ends now, on whole seconds."""
    end = int((now or datetime.now(timezone.utc)).timestamp())
    return end - hours * 3600, end


def bucket_means(changes, start, end, points=POINTS):
    """Time-weighted mean per equal part of [start, end) of a value that holds from each change to the next.

    `changes` are (unix time, value or None); None marks a time without a value (unavailable, a gap between
    statistics). A part without any known value is None."""
    changes = sorted(changes, key=lambda change: change[0])
    span = (end - start) / points
    out, index, current = [], 0, None
    while index < len(changes) and changes[index][0] <= start:
        current = changes[index][1]
        index += 1
    for part in range(points):
        begin, finish = start + part * span, start + (part + 1) * span
        moment, area, known = begin, 0.0, 0.0
        while index < len(changes) and changes[index][0] < finish:
            at, value = changes[index]
            if current is not None:
                area += current * (at - moment)
                known += at - moment
            moment, current = max(at, begin), value
            index += 1
        if current is not None:
            area += current * (finish - moment)
            known += finish - moment
        out.append(area / known if known > 0 else None)
    return out


def extremes(changes, start, end):
    """((highest, when), (lowest, when)) of the values in the range, a value from before it counting at `start`."""
    changes = sorted(changes, key=lambda change: change[0])
    seen, current = [], None
    for at, value in changes:
        if at <= start:
            current = value
            continue
        if at >= end:
            break
        if current is not None and not seen:
            seen.append((current, start))
        if value is not None:
            seen.append((value, at))
        current = value
    if not seen and current is not None:
        seen.append((current, start))
    if not seen:
        return None, None
    return max(seen, key=lambda item: item[0]), min(seen, key=lambda item: item[0])


def nice_step(span, target=3):
    """A round step (1, 2, 2.5 or 5 times a power of ten) that divides `span` into about `target` parts."""
    raw = span / target
    magnitude = 10 ** math.floor(math.log10(raw))
    for multiple in (1, 2, 2.5, 5, 10):
        if multiple * magnitude >= raw * 0.999:
            return multiple * magnitude
    return 10 * magnitude


def axis(low, high):
    """(domain, ticks): the drawn range and the round values labelled on it. A flat line gets room around it."""
    if high - low < 1e-9:
        pad = abs(high) * 0.05 or 1.0
        low, high = low - pad, high + pad
    step = nice_step(high - low)
    ticks = []
    value = math.ceil(low / step - 1e-9) * step
    while value <= high + step * 1e-9:
        ticks.append(round(value, 10))
        value += step
    if len(ticks) < 2:
        ticks = [math.floor(low / step) * step, math.ceil(high / step) * step]
    low, high = min(low, ticks[0]), max(high, ticks[-1])
    margin = (high - low) * 0.06
    return (low - margin, high + margin), ticks


def step_decimals(step):
    """The decimals a step needs to be written exactly: 0.5 needs one, 0.25 two, 2.5 one, 20 none."""
    for places in range(4):
        if abs(round(step, places) - step) <= abs(step) * 1e-6:
            return places
    return 3


def tick_text(value, step, unit):
    """An axis label: as few decimals as the step needs; degrees and percent stay on the number."""
    text = header_bar.number_text(value, step_decimals(step))
    if unit in ('°C', '°F', '°'):
        return text + '°'
    return text + '%' if unit == '%' else text


def decimals(entry, values):
    """Decimals for a value read from the line: the entity's display precision, else one for averages."""
    precision = header_bar.precision_of(entry)
    if precision is not None:
        return precision
    return 0 if all(value is None or float(value).is_integer() for value in values) else 1


def time_ticks(start, end, hours, tz):
    """Unix times of round moments in the range: every quarter hour, every six hours, or every midnight."""
    tz = tz or timezone.utc
    step = {1: timedelta(minutes=15), 24: timedelta(hours=6), 168: timedelta(days=1)}[hours]
    moment = datetime.fromtimestamp(start, tz)
    if hours == 1:
        moment = moment.replace(minute=moment.minute - moment.minute % 15, second=0, microsecond=0)
    elif hours == 24:
        moment = moment.replace(hour=moment.hour - moment.hour % 6, minute=0, second=0, microsecond=0)
    else:
        moment = moment.replace(hour=0, minute=0, second=0, microsecond=0)
    ticks = []
    # A tick right at either end would sit on the start of the line or on "Now"; the screen drops a label that
    # still collides.
    margin = (end - start) * 0.03
    while moment.timestamp() < end:
        at = moment.timestamp()
        if start + margin <= at <= end - margin:
            ticks.append(int(at))
        # Adding in local time keeps midnight on midnight across a daylight saving change.
        moment = (moment.replace(tzinfo=None) + step).replace(tzinfo=tz)
    return ticks


def utc_offset(end, tz):
    return int(((tz or timezone.utc).utcoffset(datetime.fromtimestamp(end, tz or timezone.utc)) or timedelta()).total_seconds())


def line(entity, hours, changes, start, end, tz, entry=None, unit='', extreme_changes=None):
    """The `history` message for a number: 24 averages, the highest and lowest moment, the axis and the times.

    `changes` feed the averages; `extreme_changes` (statistics' own highs and lows, or the same changes) the
    highest and lowest moment."""
    values = bucket_means(changes, start, end)
    high, low = extremes(extreme_changes if extreme_changes is not None else changes, start, end)
    known = [value for value in values if value is not None] + [item[0] for item in (high, low) if item]
    message = {'v': 1, 'op': 'history', 'entity': entity, 'hours': hours, 'kind': 'line', 'start': start, 'end': end,
               'off': utc_offset(end, tz), 'xt': time_ticks(start, end, hours, tz), 'unit': header_bar.clean_text(unit or '')[:12]}
    if not known:
        message['values'] = [None] * POINTS
        return message
    (bottom, top), ticks = axis(min(known), max(known))
    step = ticks[1] - ticks[0] if len(ticks) > 1 else 1
    places = decimals(entry, known)
    message.update({
        'values': [None if value is None else round(value, max(places, 2)) for value in values],
        'dom': [round(bottom, 6), round(top, 6)],
        'yt': [[round(tick, 6), tick_text(tick, step, unit)] for tick in ticks],
        'dec': places,
    })
    if high:
        message['hi'] = [round(high[0], max(places, 2)), int(high[1])]
    if low:
        message['lo'] = [round(low[0], max(places, 2)), int(low[1])]
    return message


def state_label(domain, state, attrs, word=None):
    """Home Assistant's words for a state, as the screen shows them elsewhere; `word` is Home Assistant's own word, for a
    state the screen has no word of its own for (app 0.2.67)."""
    if state in (None, 'unavailable'):
        return 'Unavailable'
    if state == 'unknown':
        return 'Unknown'
    if domain == 'binary_sensor':
        pair = header_bar.BINARY_STATES.get((attrs or {}).get('device_class'), ('On', 'Off'))
        return pair[0] if state == 'on' else pair[1] if state == 'off' else state
    if domain == 'person' and state not in header_bar.STATES:
        return state.replace('_', ' ')[:1].upper() + state.replace('_', ' ')[1:]
    return header_bar.STATES.get(state) or word or (state[:1].upper() + state[1:]).replace('_', ' ')


def state_color(domain, state, others):
    """A state's colour: Home Assistant's amber for on and green for home, grey for off and away; every other state
    (a zone, a status) the next colour of the palette, `others` being how many such states came before it."""
    if state in (None, 'unavailable', 'unknown'):
        return NO_DATA
    if domain in ('binary_sensor', 'switch', 'input_boolean'):
        return header_bar.AMBER if state == 'on' else OFF_GREY
    if domain == 'person' and state in ('home', 'not_home'):
        return header_bar.GREEN if state == 'home' else OFF_GREY
    return PALETTE[others % len(PALETTE)]


def timeline(entity, hours, changes, start, end, tz, attrs=None, slots=SLOTS, entry=None, translations=None):
    """The `history` message for states: the timeline as runs of slots, each state's time, and how often the
    active state began (a door opened, a person came home). `entry` and `translations` give Home Assistant's words."""
    domain = entity.split('.')[0]
    def label(state):
        return header_bar.clean_text(state_label(domain, state, attrs, state_word(entity, state, attrs, entry, translations)))[:24]
    changes = sorted(changes, key=lambda change: change[0])
    # The state in force over each stretch of the range.
    pieces, current, index = [], None, 0
    while index < len(changes) and changes[index][0] <= start:
        current = changes[index][1]
        index += 1
    moment, began = start, 0
    active = ACTIVE.get(domain)
    for at, state in changes[index:]:
        if at >= end:
            break
        if state == current:
            continue
        pieces.append((moment, at, current))
        if active and state == active:
            began += 1
        moment, current = at, state
    pieces.append((moment, end, current))
    totals, order = {}, []
    for begin, finish, state in pieces:
        if state is None:
            continue
        if state not in totals:
            order.append(state)
        totals[state] = totals.get(state, 0) + finish - begin
    # The most time first; a state beyond the sixth joins "Other".
    ranked = sorted(order, key=lambda state: -totals[state])
    shown = ranked[:STATES_SHOWN] if len(ranked) <= STATES_SHOWN else ranked[:STATES_SHOWN - 1]
    index_of = {state: i for i, state in enumerate(shown)}
    others = [state for state in order if state_color(domain, state, 0) == PALETTE[0]]
    legend = [[label(state),
               state_color(domain, state, others.index(state) if state in others else 0), int(totals[state])] for state in shown]
    rest = [state for state in ranked if state not in index_of]
    if rest:
        index_of.update({state: len(shown) for state in rest})
        legend.append(['Other', '9E9E9E', int(sum(totals[state] for state in rest))])
    # Each slot shows the state that covers most of it, except that the active state always shows: a door that
    # stood open for five minutes stays visible on a day's timeline, as it does in Home Assistant.
    span = (end - start) / slots
    runs, piece = [], 0
    for slot in range(slots):
        begin, finish = start + slot * span, start + (slot + 1) * span
        cover = {}
        while piece < len(pieces) and pieces[piece][1] <= begin:
            piece += 1
        look = piece
        while look < len(pieces) and pieces[look][0] < finish:
            p_begin, p_end, state = pieces[look]
            if state is not None:
                cover[state] = cover.get(state, 0) + min(p_end, finish) - max(p_begin, begin)
            look += 1
        winner = active if active in cover else max(cover, key=cover.get) if cover else None
        value = index_of[winner] if winner is not None else -1
        if not runs or runs[-1][1] != value:
            runs.append([slot, value])
    # Which legend entry is the active state (the card sums up its time and how often it began), or -1.
    shown_active = index_of.get(active, -1) if active in shown else -1
    return {'v': 1, 'op': 'history', 'entity': entity, 'hours': hours, 'kind': 'timeline', 'start': start, 'end': end,
            'off': utc_offset(end, tz), 'xt': time_ticks(start, end, hours, tz), 'slots': slots, 'states': legend,
            'seg': run_times(runs, pieces, index_of, start, span, slots), 'words': words(domain, order + [current], attrs, label),
            'began': began, 'active': shown_active}


def run_times(runs, pieces, index_of, start, span, slots):
    """[slot, state, begin, end, seconds] per run: a finger on a run reads the real times of its state, from its first
    moment to its last (seconds after `start`), and the time spent in it, not the width of the slots it covers."""
    out, first = [], 0
    for i, (slot, value) in enumerate(runs):
        low, high = start + slot * span, start + (runs[i + 1][0] if i + 1 < len(runs) else slots) * span
        while first < len(pieces) and pieces[first][1] <= low:
            first += 1
        found, look = [], first
        while look < len(pieces) and pieces[look][0] < high:
            begin, finish, state = pieces[look]
            if value >= 0 and state is not None and index_of.get(state) == value:
                found.append((begin, finish))
            look += 1
        if found:
            begin, finish, seconds = min(b for b, _ in found), max(f for _, f in found), sum(f - b for b, f in found)
        else:
            begin, finish, seconds = low, high, high - low
        out.append([slot, value, round(begin - start), round(finish - start), round(seconds)])
    return out


def words(domain, states, attrs, label=None):
    """[raw state, words] for the states of the range and a binary sensor's on and off: the card's heading shows the
    state now in these words, also after it changed while the card is open."""
    found = {}
    for state in states + (['on', 'off'] if domain == 'binary_sensor' else []):
        if isinstance(state, str) and len(state) <= 32 and state not in found and len(found) < 10:
            found[state] = label(state) if label else header_bar.clean_text(state_label(domain, state, attrs))[:24]
    return [[state, text] for state, text in found.items()]


def statistic_changes(rows, period_seconds):
    """Statistics rows as changes for `bucket_means` (each mean holds for its period, a gap has no value) and as
    changes for `extremes` (each period's own maximum and minimum)."""
    means, extreme = [], []
    rows = sorted((row for row in rows if isinstance(row.get('start'), (int, float))), key=lambda row: row['start'])
    for i, row in enumerate(rows):
        start = row['start'] / 1000 if row['start'] > 1e11 else row['start']
        mean = header_bar.numeric(row.get('mean') if row.get('mean') is not None else row.get('state'))
        means.append((start, mean))
        following = rows[i + 1]['start'] if i + 1 < len(rows) else None
        following = following / 1000 if following and following > 1e11 else following
        if following is None or following - start > period_seconds + 1:
            means.append((start + period_seconds, None))
        for key in ('max', 'min'):
            value = header_bar.numeric(row.get(key))
            if value is not None:
                extreme.append((start, value))
        if mean is not None and header_bar.numeric(row.get('max')) is None:
            extreme.append((start, mean))
    return means, extreme
