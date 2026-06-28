# Lily58 Rev1 — HK Keymap Build Notes

## Hardware

| Component | Detail |
|-----------|--------|
| Keyboard | Lily58 Rev1 |
| MCU | RP2040 |
| Pointing device | Azoteq TPS43 (right side) |
| Display | SSD1306 OLED |
| RGB | WS2812 (GP0) |
| Split comms | Half-duplex serial, GP1 |
| Diode direction | COL2ROW — stripe/band faces the **row** side |
| USB master | Left half |

### Matrix Pins

| Type | Pins |
|------|------|
| Rows | GP5, GP6, GP7, GP8, GP9 |
| Cols | GP27, GP26, GP22, GP20, GP23, GP21 |

Rows 0–4 = left half, rows 5–9 = right half.

---

## Relevant Files

```
qmk_firmware/
├── keyboards/lily58/
│   ├── rev1/
│   │   ├── keyboard.json       # Matrix pins, split, RGB, OLED, layout
│   │   └── halconf.h           # Enables HAL_USE_I2C for OLED
│   └── keymaps/hk/
│       ├── keymap.c            # Layer definitions (edit this for layout changes)
│       ├── config.h            # MASTER_LEFT defined here (overridden by holykeebs)
│       └── rules.mk            # VIA, OLED, MOUSEKEY, EXTRAKEY enables
└── users/holykeebs/
    ├── config.h                # MASTER_RIGHT/LEFT, SERIAL_USART_TX_PIN GP1, split watchdog
    ├── rules.mk                # Pointing device configs, OLED, SERIAL_DRIVER=vendor
    ├── holykeebs.h / .c        # User-space feature implementations
    └── oled.c                  # HK OLED rendering
```

---

## Build Commands

### Standard build

```bash
cd ~/Project/Lily58/qmk_firmware
make lily58/rev1:hk POINTING_DEVICE=tps43 POINTING_DEVICE_POSITION=right OLED=yes USER_NAME=holykeebs
```

Output: `.build/lily58_rev1_hk.uf2`

### Clean build (use after editing keymap.c or config files)

```bash
make lily58/rev1:hk:clean
make lily58/rev1:hk POINTING_DEVICE=tps43 POINTING_DEVICE_POSITION=right OLED=yes USER_NAME=holykeebs
```

### Flash

1. Double-tap the reset button — a `RPI-RP2` drive will appear
2. Copy the `.uf2` to that drive:

```bash
cp .build/lily58_rev1_hk.uf2 /media/$USER/RPI-RP2/
```

The keyboard reboots automatically once the file is copied.

---

## Make Variables Reference

| Variable | Value | Purpose |
|----------|-------|---------|
| `POINTING_DEVICE` | `tps43` | Enables Azoteq IQS5xx driver |
| `POINTING_DEVICE_POSITION` | `right` | TPS43 is on the right half |
| `OLED` | `yes` | Enables OLED with holykeebs renderer |
| `USER_NAME` | `holykeebs` | Loads `users/holykeebs/` rules and config |

> `keymaps/hk/config.h` sets `DYNAMIC_KEYMAP_LAYER_COUNT 6` — VIA's default of 4 isn't
> enough now that the GAMING layer brings the total to 6. If a future keymap edit hits
> `Number of keymap layers exceeds maximum set by DYNAMIC_KEYMAP_LAYER_COUNT`, bump this.
> Remember: bumping this changes VIA's EEPROM layout, so you must clear EEPROM (`EECLR`
> on the ADJUST layer) after flashing for the new layer to actually take effect.

> **Critical:** `USER_NAME=holykeebs` must be passed. QMK defaults `USER_NAME` to the
> keymap name (`hk`), so without this, `users/holykeebs/rules.mk` and
> `users/holykeebs/config.h` are never loaded. This causes:
> - `SOFT_SERIAL_PIN` build error (wrong serial driver selected)
> - Right half not communicating (no `SERIAL_USART_TX_PIN GP1`)
> - Wrong master side (holykeebs overrides `MASTER_LEFT` with `MASTER_RIGHT`)

---

## Layer Map

### Layer 0 — QWERTY (base)

```
ESC  1    2    3    4    5              6    7    8    9    0    `
Tab  Q    W    E    R    T              Y    U    I    O    P    -
Ctrl A    S    D    F    G              H    J    K    L    ;    '
Shft Z    X    C    V    B   [      ]   N    M    ,    .    /   RShft
               Lwr  GUI  Alt  NAV_Spc Ent  Bksp GUI  Rse
```

> Ctrl and Shift are in standard position — Ctrl on the home row, Shift on the bottom row.

### Layer 1 — LOWER (hold `Lwr`)

```
---  ---  ---  ---  ---  ---            ---  ---  ---  ---  ---  ---
---  !    @    #    $    %              ^    &    *    (    )    ---
---  1    2    3    4    5              6    7    8    9    0    ---
---  D_MD B4   B5   B1   B2  S_MD  ---  |    `    +    {    }   ---
```

Mouse buttons (B1/B2/B4/B5) and HK trackpad mode keys (D_MD, S_MD).

### Layer 2 — RAISE (hold `Rse`)

```
---  ---  ---  ---  ---  ---            ---  ---  ---  ---  ---  ---
F1   F2   F3   F4   F5   F6             F7   F8   F9   F10  F11  F12
---  ---  ---  ---  ---  ---            ---  Left Down Up   Rght ---
---  ---  ---  ---  ---  ---  ---  ---  +    =    [    ]    \   ---
```

### Layer 3 — ADJUST (hold `Lwr` + `Rse`)

```
---   ---      ---      ---       ---  ---                ---  ---  ---  ---  ---  ---
BOOT  HK_DUMP  HK_SAVE  HK_RESET  ---  HK_C_SCROLL        ---  ---  ---  ---  ---  BOOT
EECLR HK_P_D   HK_P_S   HK_P_BUF  ---  HK_S_MODE_T        Up   Dn   ---  ---  ---  EECLR
Shft  ---      ---      ---       ---  HK_D_MODE_T  ---  ---  ---  ---  ---  ---  ---  ---
```

`BOOT` = enter bootloader. `EECLR` = wipe EEPROM/VIA settings.

### Layer 4 — NAV (hold left thumb `Space`)

```
---  ---  ---  ---  ---  ---            ---  ---   ---  ---   ---  ---
---  ---  ---  ---  ---  ---            PgUp Home  Up   End   ---  ---
---  ---  ---  ---  ---  ---            PgDn Left  Down Right ---  ---
---  ---  ---  ---  ---  ---  ---  ---  ---  ---   ---  ---   ---  ---
```

IJKL-style arrows (mirrors the WASD inverted-T shape) on the right hand, with
Home/End/PgUp/PgDn flanking. Bound to the left thumb's Space key via
`LT(_NAV, KC_SPC)` — tap sends Space as normal; hold (or hold + another key)
activates the layer without ever sending a Space keypress. Left hand holds,
right hand navigates.

### Layer 5 — GAMING (toggle: press `[` and `]` together)

```
---  ---  ---  ---  ---  ---            ---  ---  ---  ---  ---  ---
---  ---  ---  ---  ---  ---            ---  ---  ---  ---  ---  ---
---  ---  ---  ---  ---  ---            ---  ---  ---  ---  ---  ---
---  ---  ---  ---  ---  ---  ---  ---  ---  ---  ---  ---  ---  ---
                NO   GUI  Alt  Space  Ent  Bksp  GUI   NO
```

Mostly transparent to QWERTY — letters, WASD, Ctrl, Shift all stay exactly
where they are. Toggled on and off by pressing the `[` and `]` keys
simultaneously (a QMK combo bound to `TG(_GAMING)`), so no held key is
needed to stay in the layer. While active:

- `LOWER` and `RAISE` thumb keys are disabled (`KC_NO`) so layer-shift keys
  can't fire accidentally during a game.
- The `NAV_SPC` thumb key reverts to plain `KC_SPC` (e.g. for jump), since
  holding Space to reach NAV would otherwise interfere with gameplay.

---

## Key Matrix Positions

Useful when debugging a key not registering — check row/column on the OLED debug display (`ROWxCOL, kKEYCODE : name`).

| Key | Matrix | Row GPIO | Col GPIO |
|-----|--------|----------|----------|
| LShift | [3,0] | GP8 | GP27 |
| LCtrl | [2,0] | GP7 | GP27 |
| K | [7,3] | GP8 | GP20 |
| Enter | [9,4] | GP9 | GP23 |
| Space | [4,4] | GP9 | GP23 |
| Backspace | [9,3] | GP9 | GP20 |
| RAISE | [9,1] | GP9 | GP26 |
| LOWER | [4,1] | GP4 | GP26 |

---

## Troubleshooting

### Ghost typing on plug-in (no keys pressed)

Caused by floating row/column pins at empty switch positions.

- Clean the PCB with 90%+ isopropyl alcohol (flux residue creates leakage paths)
- Inspect under magnification for solder bridges near the affected positions
- Verify diode orientation: COL2ROW means the **stripe/band faces the row side**
- This is expected behaviour when testing with only some switches installed

### `SOFT_SERIAL_PIN` undeclared build error

```
platforms/chibios/drivers/serial.c: error: 'SOFT_SERIAL_PIN' undeclared
```

Missing `USER_NAME=holykeebs` in the build command. Without it, `users/holykeebs/rules.mk`
is never loaded and `SERIAL_DRIVER` defaults to `bitbang`. Fix:

```bash
make lily58/rev1:hk ... USER_NAME=holykeebs
```

### Right half not responding after flash

`users/holykeebs/config.h` defines `SERIAL_USART_TX_PIN GP1` for split communication.
Without `USER_NAME=holykeebs`, this pin is never configured and the halves cannot talk.
Solution: same as above — always pass `USER_NAME=holykeebs`.

### Keymap change not taking effect after rebuild

The build cache can retain stale object files. Always clean before reflashing after a keymap edit:

```bash
make lily58/rev1:hk:clean
make lily58/rev1:hk POINTING_DEVICE=tps43 POINTING_DEVICE_POSITION=right OLED=yes USER_NAME=holykeebs
```

### Trackpad X/Y axis inverted intermittently, or trackpad dead on plug-in

Both are fixed as of this firmware — see `FIXES.md` for the full root-cause
writeup. Short version: `users/holykeebs/config.h`'s `SPLIT_WATCHDOG_ENABLE`
can trigger repeated MCU resets on the right half without power-cycling the
IQS5xx trackpad chip, which re-runs `azoteq_iqs5xx_init()` under marginal I2C
timing. This previously caused (a) toggle-based axis flip bits to drift with
each unintended re-init, and (b) a single missed product-detection check to
permanently disable the trackpad for the session. Both are now idempotent/
retried in `drivers/sensors/azoteq_iqs5xx.c`. If either symptom recurs after a
clean flash, it's a regression worth re-checking against that file.

### Key registering wrong keycode

Use the OLED debug display — it shows `ROWxCOL, kKEYCODE : name` for the last pressed key.
Cross-reference the row/col against the matrix table above to identify whether the issue
is the switch, diode, or a solder bridge on that row/column.
