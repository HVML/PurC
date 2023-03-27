# Foil Specific CSS Properties

## CSS3 Properties Supported by Foil

### `appearance`

Foil supports standard keywords for `appearance` property:

- Since 0.9.6
   - `auto`
   - `none`

Foil supports non-standard keywords for `appearance` property:

- Since 0.9.6
  - `progress-bkgnd`
  - `progress-bar`, `auto` for `PROGRESS` element.
  - `progress-mark`
  - `meter`, `meter-bar`, and `auto` for `METER` element.
  - `meter-bkgnd`
  - `meter-mark`

### `-foil-color-*`

Theses color properties can be used to render a UI control defined by a `progress`, `meter`, `input`, or `select` element.

- `-foil-color-info`: The color for neutral and informative content.
- `-foil-color-warning`: The color used for non-destructive warning messages.
- `-foil-color-danger`: The color used for errors and dangerous actions.
- `-foil-color-success`: The color used for positive or successful actions and information.
- `-foil-color-primary`: The main color, used for hyperlinks, focus styles, and component and form active states.

The definition is same as `color` property, but they are not inherited.

- Name: `-foil-color-info`
- Value: `<color>` | `default` | `inherit`
- Initial: depends on user agent
- Applies to: all elements
- Inherited: no
- Percentages: N/A
- Media: visual
- Computed value: as specified

### `-foil-candidate-marks`

If the appearance is `progress-mark` or `meter-mark`, this property defines the candidate marks used for mark indicator.

If the appearance is `progress-bar` or `meter`/`meter-bar`, this property defines the mark used to render the bar.

Definition:

- Initial value: auto
- Applies to: all elements
- Inherited: no
- Computed value: A string contains a list of characters can be used as the marks of a progress or meter.
- Animation type: discrete

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
  - `voice-family`
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

## Extended Properties

### New keyword for color properties

Foil supports the following non-standard keywords for color properties:

- `default`: The default background color or foreground color of the terminal.

### `list-style-type`

Foil supports the following keywords defined in CSS3:

- `upper-armenian`
- `lower-armenina`
- `cjk-decimal`
- `tibetan`

