# Omaunits — Implementation Plan

Council-approved design (Claude Opus · Codex GPT-6.1-Sol xhigh · Grok 4.7;
blind round → debate → tie-break, all members COMMITTED). Debate record in
`council/`.

## Amendments (owner decisions after council approval)

> The spec body below predates these amendments and the round-1 review fixes;
> where it disagrees with this section, this section and the code win.

1. **Currency removed.** The product ships five categories — weight, length,
   temperature, volume, data. Unit symbols (kg, lb, kB) are universal and
   the rate pipeline was the app's only network dependency; both went.
2. **i18n scoped to the category picker.** Only category names and the
   picker's filter placeholder translate; unit symbols and names stay
   universal/English. Dictionaries: `i18n/omaunits.<lang>.json`.
3. **Visible seam swap button.** The council's keypad-only swap hid the
   converter's main action; a mouse-friendly swap button now rides the
   hairline between the two value rows (the inverted keypad key stays).
4. **Rate line moved to its own full-width row** below the category button,
   fixing elision of long notes.


## Product

A single-window, keyboard-first unit converter that reads as Omacalc's
sibling: type a number, the conversion happens as you type, no equals key.
Qt 6 Quick + C++17, qmake, MIT, no dependencies beyond Qt (+ `network`).

## Categories & units

| Category | Base | Units |
|---|---|---|
| Weight | kg | mg, g, oz, lb, st, kg, t (avoirdupois, stone, metric tonne) |
| Length | m | mm, cm, m, km, in, ft, yd, mi (international: in = 0.0254 m) |
| Temperature | K | °C, °F, K (affine) |
| Currency | EUR | every code in the ECB snapshot (~30, dynamic), 12 majors pinned: USD EUR GBP JPY AUD CAD CHF CNY INR SEK NOK DKK, rest alphabetical |
| Volume | L | mL, L, tsp, tbsp, US cup, US fl oz, US gal (US customary, labeled) |
| Data | B | B, kB, MB, GB, TB, KiB, MiB, GiB, TiB (both families — GB vs GiB is the point) |

## Window (400 × 568 design size, min 300 × 430)

```
┌──────────────────────────────────────────┐
│ ‹ weight ›                1 kg = 2.20462 lb │ category header · muted rate line (21px)
│                                          │
│  [kg ▾]                           72.5   │ input row: unit chip + entry (~40px, right)
│  [lb ▾]                         159.835  │ result row (76px, HorizontalFit)
│──────────────────────────────────────────│ hairline, 16% mix
│   7    8    9    ⌫                       │ keypad 4×4, ~58% of height
│   4    5    6    AC                      │
│   1    2    3    ±                       │
│   0    .   [      ⇅ swap      ]          │ swap = inverted key (2 cells)
└──────────────────────────────────────────┘
```

- Keypad is pure digits, always — identical contract to Omacalc.
- The muted statement line shows the input statement `1 kg → lb`; the huge
  number is the bare result (no unit symbol — the statement carries symbols).
- The rate line doubles as the conversion teacher (`1 kg = 2.20462 lb`) and
  the currency freshness line.
- The focused unit chip inverts (ink fill); default focus is the destination,
  because the usual question is "show this in another unit".

## Interaction

- **Live conversion as you type**; entry machine copied from Omacalc
  (15-significant-digit cap, leading-zero replace, lingering `1.`,
  comma-as-dot, typographic minus accepted on paste).
- **Swap keeps the typed number** (10 kg→lb becomes 10 lb→kg). The result is
  never fed back; swap-swap returns the original string.
- **Unit picking — one mechanism:** a themed popup for units and categories.
  Click a chip or the `‹ category ›` header (or Space / Shift+Space / Enter on
  a focused chip). Type to filter by symbol, name, or alias; exact symbol
  matches rank first (`gi` → GiB, `g` → gram); ↑/↓ navigate, Enter commits,
  Esc cancels. Same unit on both sides is legal and short-circuits to the
  input untouched.
- **Category:** ←/→ cycles, Ctrl+1…6 jumps, header click opens the picker.
  Changing category keeps the typed number and restores that category's last
  pair.
- **Landing:** first run Weight, kg→lb, entry `1` selected so the first digit
  replaces it. Currency defaults from the locale (locale currency → USD, or
  → EUR if the locale is USD). Later launches restore the last category and
  per-category pairs; the value always resets to `1`.
- **Errors:** below absolute zero shows an inline "Below absolute zero";
  non-finite shows `Error`, next digit recovers.

### Keyboard map

| Keys | Action |
|---|---|
| `0–9`, `.`, `,` | enter digits |
| `Backspace` | delete last digit |
| `C`, `Delete`, `Esc` | clear (Esc closes the popup first) |
| `S`, `-` | toggle sign |
| `X` | swap |
| `Space` / `Shift+Space` | open result / input unit picker |
| `Enter`, `=` | commit open picker, else copy the result |
| `←`/`→` | cycle category |
| `↑`/`↓` | cycle focused unit (`Shift` for input unit) |
| `Ctrl+1…6` | jump to category |
| `Tab` / `Shift+Tab` | move focus between unit chips |
| `Ctrl+C` / `Super+C` | copy result; `Ctrl+Shift+C` copies `72.5 kg = 159.835 lb` |
| `Ctrl+V` / `Super+V` | paste a number |
| `Ctrl+R` | force currency refresh |
| `Ctrl+Q` | quit |

## Currency

- **Source:** ECB `eurofxref-daily.xml`, parsed with `QXmlStreamReader`.
  No API key; the only new Qt module is `network`.
- **Bundled seed** in the qrc (`data/eurofxref-daily.xml`), refreshed by
  `bin/update-rates` before releases. **Cache:** the raw XML at
  `QStandardPaths::CacheLocation` (`~/.cache/omaunits/`), written with
  `QSaveFile` only after a successful parse; the file's mtime is the fetch
  time. Bundled, cached, and fetched bytes share one parser; the newest valid
  `<Cube time>` wins; date regressions are rejected.
- **Lazy fetching:** the app opens a socket only the first time Currency is
  entered in a session AND the cache is older than 6 h; re-checks every 6 h
  only while Currency is on screen; 10 s transfer timeout, no retry loops;
  `Ctrl+R` forces. A weight session never touches the network.
- **Freshness:** the rate line always shows the ECB date
  (`1 EUR = 1.0842 USD · ECB 30 Sep`); at ≥ 4 calendar days it appends
  `STALE · Nd` in the theme accent; a failed refresh notes the failure while
  conversion continues on the retained snapshot. Age is measured from the ECB
  date, so a successful fetch never resets it.
- **Math:** `y = x × rate[to] / rate[from]`, EUR = 1. Non-positive or
  incomplete parses drop the file and keep the last good snapshot. If a saved
  code disappears from the ECB list, fall back to the default pair.

## Data model & code layout

```cpp
struct Unit { QString id, symbol, name; QStringList aliases;
              double scale, offset = 0; };
// base = value * scale + offset ; out = (base - offsetTo) / scaleTo
```

- `src/systemtheme.{h,cpp}` — copied verbatim from Omacalc (portal dark mode
  + text scale).
- `src/omarchytheme.{h,cpp}` — Omacalc's colors.toml loading and re-armed
  watchers, extracted from its Backend with identical behavior.
- `src/unitcatalog.h` — static, no QObject: curated constexpr tables,
  `categories()`, `convert()`, identity short-circuit.
- `src/rates.{h,cpp}` — `RateStore : QObject`: seed/cache load, ECB parse,
  async fetch, `ratesChanged`.
- `src/backend.{h,cpp}` — the QML facade in Omacalc's shape: entry state,
  `pressKey`, swap, cycle, copy/paste, formatting, `QSettings` (org `Omacom`,
  app `omaunits` — cannot clobber omacalc), geometry, theme properties.
- `src/main.cpp` — Omacalc's font/portal wiring with names changed.
- QML: `Main.qml` (window, theme mixing, uiScale, shortcuts, geometry),
  `ConvertButton.qml` (CalcButton + `primary` kind + canvas swap icon),
  `ValueRow.qml`, `CategoryHeader.qml`, `UnitPicker.qml`.
- Formatting: quantities at 10 significant digits (scientific for extremes,
  negative zero suppressed, trailing zeros trimmed); currency at 2 decimals,
  or 4 significant digits below 1. Copy uses the displayed string; math never
  reads formatted output.

```
omaunits/
├── LICENSE  README.md  omaunits.pro
├── bin/update-rates
├── fonts/            (iA Writer Mono S Regular+Bold, OFL.txt — from omacalc)
├── data/eurofxref-daily.xml
├── src/              (main, backend, omarchytheme, systemtheme, unitcatalog,
│                      rates, resources.qrc, Main/ConvertButton/ValueRow/
│                      CategoryHeader/UnitPicker .qml)
└── tests/            (tests.pro, tst_omaunits.cpp)
```

## Tests (QtTest, Omacalc's tst style: temp QSettings path, HOME override)

- Reference points: −40 °C = −40 °F; 100 °C = 212 °F; 0 K = −459.67 °F;
  1 lb = 16 oz; 1 in = 25.4 mm; 1 GiB = 1024 MiB; 1 mi = 1.609344 km.
- Every linear factor round-trips within 1e-12; identity pairs return the
  input string untouched; swap-swap restores the original.
- Below absolute zero; non-finite recovery; formatting oracle strings.
- ECB fixture parses; malformed files rejected; newer cache beats bundled;
  date regression rejected; freshness with an injected clock.
- Picker ranking (g vs kg, GB vs GiB); key handling; launch state locked
  (`1 kg → lb`); settings persistence isolated from omacalc.

## Build & verify

1. qmake6 && make; run `tests` target; launch on this Omarchy machine and
   confirm theme re-tint, dark mode, text scale, geometry persistence.
2. `bin/update-rates` refreshes the seed; a screenshot lands in
   `screenshots/`.

## Chair's completions (details the council left open, consistent with rulings)

- Keypad arrangement: right column `⌫ AC ±`, bottom row `0 . ⇅(2 cells)`.
- Ctrl+1…6 order follows the category table above.
