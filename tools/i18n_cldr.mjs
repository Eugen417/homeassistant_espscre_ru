// Day and month names, date orders, the decimal mark and the clock of languages, from the Unicode CLDR that Node's Intl
// carries (app 0.2.90); tools/i18n.py cldr writes them into the translations. Prints JSON: {code: {...}}.
const codes = process.argv.slice(2);
const space = (text) => text.replace(/[   ]/g, ' ');
// 2026-03-01 is a Sunday: day i of that week is Sunday + i, as the screens count (ESPHome's day_of_week 1 is Sunday).
const day = (i) => new Date(Date.UTC(2026, 2, 1 + i, 12));
const month = (m) => new Date(Date.UTC(2026, m, 14, 12));
const pattern = (code, options, names) => {
  const parts = new Intl.DateTimeFormat(code, { ...options, timeZone: 'UTC' }).formatToParts(new Date(Date.UTC(2026, 8, 14, 14, 5)));
  let out = '';
  for (const part of parts) {
    if (names[part.type]) out += `{${names[part.type]}}`;
    else if (part.type === 'literal') out += space(part.value);
  }
  return out.replace(/\{hour\}:\{minute\}/, '{time}').replace(/\{hour\}\.\{minute\}/, '{time}');
};
const result = {};
for (const code of codes) {
  const short = new Intl.DateTimeFormat(code, { weekday: 'short', timeZone: 'UTC' });
  const long = new Intl.DateTimeFormat(code, { weekday: 'long', timeZone: 'UTC' });
  const monthLong = (m) => new Intl.DateTimeFormat(code, { day: 'numeric', month: 'long', timeZone: 'UTC' }).formatToParts(month(m)).find((p) => p.type === 'month').value;
  const monthShort = (m) => new Intl.DateTimeFormat(code, { day: 'numeric', month: 'short', timeZone: 'UTC' }).formatToParts(month(m)).find((p) => p.type === 'month').value;
  const numbers = new Intl.NumberFormat(code);
  const parts = numbers.formatToParts(12345.5);
  const decimal = parts.find((p) => p.type === 'decimal').value;
  const group = space(parts.find((p) => p.type === 'group').value);
  const period = (hour) => (new Intl.DateTimeFormat(code, { hour: 'numeric', hour12: true, timeZone: 'UTC' })
    .formatToParts(new Date(Date.UTC(2026, 0, 1, hour))).find((p) => p.type === 'dayPeriod') || { value: hour < 12 ? 'AM' : 'PM' }).value;
  const cycle = new Intl.DateTimeFormat(code, { hour: 'numeric', timeZone: 'UTC' }).resolvedOptions().hourCycle;
  // A locale that writes a short date with the month as a number (Portugal: "14/09") gets the standalone short names
  // and "day month" instead: the screens' calendar shows a month's name beside the big day.
  const numericMonth = /^\d+$/.test(monthShort(8));
  const standalone = (m) => space(new Intl.DateTimeFormat(code, { month: 'short', timeZone: 'UTC' }).format(month(m)));
  const withNames = (text) => (numericMonth ? text.replace(/\{day\}\W*\{month\}/, '{day} {month}') : text);
  result[code] = {
    clock: cycle === 'h12' || cycle === 'h11' ? '12' : '24',
    number: { decimal, group, group_min: numbers.format(1234).includes(parts.find((p) => p.type === 'group').value) ? '1' : '2' },
    time: { am: space(period(9)), pm: space(period(21)) },
    date: {
      weekdays: [0, 1, 2, 3, 4, 5, 6].map((i) => long.format(day(i))),
      weekdays_short: [0, 1, 2, 3, 4, 5, 6].map((i) => short.format(day(i))),
      // The narrow columns of the weather card hold two letters: a first guess from the short name without its dot
      // ("sö", "må" in Swedish, as the CLDR's short width), which the language's translator checks (tools/i18n.py keeps
      // the list once a language has it).
      weekdays_min: [0, 1, 2, 3, 4, 5, 6].map((i) => [...short.format(day(i)).replace(/\.$/, '')].slice(0, 2).join('')),
      months: [...Array(12).keys()].map(monthLong),
      months_short: [...Array(12).keys()].map(numericMonth ? standalone : monthShort),
      top_bar: withNames(pattern(code, { weekday: 'short', day: 'numeric', month: 'short' }, { weekday: 'weekday', day: 'day', month: 'month' })),
      full: pattern(code, { weekday: 'long', day: 'numeric', month: 'long' }, { weekday: 'weekday', day: 'day', month: 'month' }),
      day_month: withNames(pattern(code, { day: 'numeric', month: 'short' }, { day: 'day', month: 'month' })),
      weekday_time: pattern(code, { weekday: 'short', hour: '2-digit', minute: '2-digit', hourCycle: 'h23' }, { weekday: 'weekday', hour: 'hour', minute: 'minute' }),
    },
  };
}
console.log(JSON.stringify(result, null, 1));
