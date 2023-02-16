# Foil Specific CSS Properties

## CSS3 Properties Supported by Foil

### `appearance`

Foil supports standard keywords for `appearance` property:

- `auto`:
- `progress-bar`:
- `meter`:

Foil supports non-standard keywords for `appearance` property:

- `symbol-indicator`

### `-foil-color-*`

- `-foil-color-info`: The color for neutral and informative content.
- `-foil-color-warning`: The color used for non-destructive warning messages.
- `-foil-color-danger`: The color used for errors and dangerous actions.
- `-foil-color-success`: The color used for positive or successful actions and information.
- `-foil-color-primary`: The main color, used for hyperlinks, focus styles, and component and form active states.

### `-foil-candidate-symbols`

If the appearance is `symbol-indicator`, this property defines the candidate symbols used for symbol indicator.

If the appearance is `progress-bar` or `meter`, this property defines the symbol used to render the bar.

## CSS2 Properties not Supported by Foil

The following properties of CSS2 are not supported by Foil:

- All properties for aural media:
  - `cue-xxx`
  - `elevation`
  - `pause-xxx`
  - `pitch-xxx`
  - `play-during`
  - `richness`
  - `speak-xxx`
  - `stress`
  - voice-family`
  - `volume`
- Properties only work for GUI:
  - `background-attachment`
  - `background-image`
  - `background-position`
  - `background-repeat`
  - `list-style-image`
  - `cursor`
  - `font-family`
  - `font-size`
  - `font-style`
  - `font-variant`
  - `font-weight`
  - `font`
  - `outline-color`
  - `outline-style`
  - `outline-width`
  - `outline`
- Properties only work for paged media:
  - `orphans`
  - `page-break-after`
  - `page-break-before`
  - `page-break-inside`
  - `widows`

## Extended CSS2 Properties

### `background-color`

Foil supports non-standard keywords for `background-color` property:

- `default`: The default background color of the terminal.

### `color`

Foil supports non-standard keywords for `color` property:

- `default`: The default foreground color of the terminal.

### `list-style-type`

Foil supports the following keywords defined in CSS3:

- `upper-armenian`
- `lower-armenina`
- `cjk-decimal`
- `tibetan`

