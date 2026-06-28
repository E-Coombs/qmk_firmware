#include "pointing.h"
#include "quantum.h"
#include <string.h>
#ifdef WPM_ENABLE
#    include "wpm.h"
#endif

static const char* pointer_kind_to_string(hk_pointer_kind kind) {
    switch (kind) {
        case POINTER_KIND_NONE:
            return "NONE \xB1";
        case POINTER_KIND_PIMORONI_TRACKBALL:
            return "PIM  \xB1";
        case POINTER_KIND_TRACKPOINT:
            return "TP   \xB1";
        case POINTER_KIND_CIRQUE35:
            return "CR35 \xB1";
        case POINTER_KIND_CIRQUE40:
            return "CR40 \xB1";
        case POINTER_KIND_TPS43:
            return "TPS43\xB1";
        default:
            return "?????\xB1";
    }
}

static const char BL = '\xB0'; // Blank indicator character
static const char LFSTR_ON[] PROGMEM = "\xB2\xB3";
static const char LFSTR_OFF[] PROGMEM = "\xB4\xB5";

static const char *format_3d(int8_t d) {
    static char buf[10] = {0};
    char        lead   = ' ';
    if (d < 0) {
        d    = -d;
        lead = '-';
    }
    sprintf(buf, "%c%2d", lead, d);
    return buf;
}

static const char *format_2d(int8_t d) {
    static char buf[10] = {0};
    if (d > 99) {
        d = 99;
    }
    sprintf(buf, "%02d",d);
    return buf;
}

static char to_1x(uint8_t x) {
    x &= 0x0f;
    return x < 10 ? x + '0' : x + 'a' - 10;
}

static const char* format_multiplier(float f) {
    static char buf[10] = {0};
    if (f > 10 || f < -10) {
        sprintf(buf, "err");
    } else {
        sprintf(buf, "%2.1f", f);
    }
    return buf;
}

void hk_oled_render_pointer_state(void) {
    // Output example:
    //
    //  TPS43: -12  34   0   0
    //  CUR D: 1.0/5  ON LK:VT

    // 1st line, pointing device kind, mouse x, y, h, and v.
    oled_write_P(pointer_kind_to_string(g_hk_state.main.pointer_kind), false);

    oled_write(format_3d(g_hk_state.display.last_mouse.x), false);
    oled_write(format_3d(g_hk_state.display.last_mouse.y), false);
    oled_write(format_3d(g_hk_state.display.last_mouse.h), false);
    oled_write_ln(format_3d(g_hk_state.display.last_mouse.v), false);

    // 2nd line, cursor mode, default multiplier, drag scroll mode, scroll lock mode, and scroll buffer size.
    if (g_hk_state.setting_default_scale) {
        oled_write_P(PSTR("CUR D\xB1"), false);
        oled_write(format_multiplier(g_hk_state.main.pointer_default_multiplier), false);
    } else if (g_hk_state.setting_sniping_scale) {
        oled_write_P(PSTR("CUR S\xB1"), false);
        oled_write(format_multiplier(g_hk_state.main.pointer_sniping_multiplier), false);
    } else {
        switch (g_hk_state.main.cursor_mode)
        {
            case CURSOR_MODE_DEFAULT:
                oled_write_P(PSTR("CUR D\xB1"), false);
                oled_write(format_multiplier(g_hk_state.main.pointer_default_multiplier), false);
                break;
            case CURSOR_MODE_SNIPING:
                oled_write_P(PSTR("CUR S\xB1"), false);
                oled_write(format_multiplier(g_hk_state.main.pointer_sniping_multiplier), false);
                break;
            default:
                oled_write_P(PSTR("CUR ?\xB1"), false);
                break;
        }
    }

    oled_write_char('/', false);
    // scroll buffer size:
    oled_write(format_2d(g_hk_state.main.pointer_scroll_buffer_size), false);
    oled_write_char(' ', false);

    // drag scroll mode: on/off
    if (g_hk_state.main.drag_scroll) {
        oled_write_P(LFSTR_ON, false);
    } else {
        oled_write_P(LFSTR_OFF, false);
    }

    // scroll lock mode: "VT" (vertical), "HN" (horiozntal), and "NO" (free)
    switch (g_hk_state.main.scroll_lock) {
        case SCROLL_LOCK_VERTICAL:
            oled_write_ln_P(PSTR(" L:VT"), false);
            break;
        case SCROLL_LOCK_HORIZONTAL:
            oled_write_ln_P(PSTR(" L:HO"), false);
            break;
        default:
            oled_write_ln_P(PSTR(" L:NO"), false);
            break;
    }
}

void hk_oled_render_keyinfo(void) {
    // Format: `Key :  R{row}  C{col} K{kc} {name}{name}{name}`
    //
    // Where `kc` is lower 8 bit of keycode.
    // Where `name`s are readable labels for pressing keys, valid between 4 and 56.
    //
    // `row`, `col`, and `kc` indicates the last processed key,
    // but `name`s indicate unreleased keys in best effort.
    //
    // For example:
    //
    //     Key  :  R2  C3 K06 abc
    //     TPS43:   0   0   0   0

    // "Key" Label.
    oled_write_P(PSTR("Key  \xB1"), false);

    // Row and column.
    oled_write_char('\xB8', false);
    oled_write_char(to_1x(g_hk_state.display.last_pos.row), false);
    oled_write_char('\xB9', false);
    oled_write_char(to_1x(g_hk_state.display.last_pos.col), false);

    // Keycode.
    oled_write_P(PSTR("\xBA\xBB"), false);
    oled_write_char(to_1x(g_hk_state.display.last_kc >> 4), false);
    oled_write_char(to_1x(g_hk_state.display.last_kc), false);

    // Keys currently pressed.
    oled_write_P(PSTR(" "), false);
    oled_write(g_hk_state.display.pressing_keys, false);
}

void hk_oled_render_layerinfo(void) {
    // Format: `Layer:{layer state}`
    //
    // Output example:
    //
    //     Layer:-23------------
    //
    oled_write_P(PSTR("Layer\xB1"), false);
    for (uint8_t i = 1; i < 8; i++) {
        oled_write_char((layer_state_is(i) ? to_1x(i) : BL), false);
    }
    oled_write_char(' ', false);

#    ifdef POINTING_DEVICE_AUTO_MOUSE_ENABLE
    oled_write_P(PSTR("\xC2\xC3"), false);
    if (get_auto_mouse_enable()) {
        oled_write_P(LFSTR_ON, false);
    } else {
        oled_write_P(LFSTR_OFF, false);
    }

    oled_write(format_3d(get_auto_mouse_timeout()), false);
    oled_write_char('0', false);
#    else
    oled_write_P(PSTR("\xC2\xC3\xB4\xB5 --"), false);
#    endif
}

#if defined(HK_OLED_PORTRAIT_BANNER)
// Portrait, icon-based display for boards mounted with the OLED rotated 90/270 degrees.
// Opt in per-keymap via `#define HK_OLED_PORTRAIT_BANNER` (e.g. keymaps/hk/config.h for
// lily58) — left off by default so other holykeebs boards keep the dense debug view above.

static void oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on) {
    for (uint8_t yy = y; yy < y + h; yy++) {
        for (uint8_t xx = x; xx < x + w; xx++) {
            oled_write_pixel(xx, yy, on);
        }
    }
}

static void oled_fill_ring(uint8_t cx, uint8_t cy, uint8_t r_outer, uint8_t r_inner) {
    int16_t ro2 = (int16_t)r_outer * r_outer;
    int16_t ri2 = (int16_t)r_inner * r_inner;
    for (int16_t dy = -r_outer; dy <= r_outer; dy++) {
        for (int16_t dx = -r_outer; dx <= r_outer; dx++) {
            int16_t d2 = dx * dx + dy * dy;
            if (d2 <= ro2 && d2 >= ri2) {
                int16_t x = cx + dx, y = cy + dy;
                if (x >= 0 && y >= 0) {
                    oled_write_pixel((uint8_t)x, (uint8_t)y, true);
                }
            }
        }
    }
}

static void oled_fill_disc(uint8_t cx, uint8_t cy, uint8_t r) {
    oled_fill_ring(cx, cy, r, 0);
}

static void oled_fill_chevron_down(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    for (uint8_t row = 0; row < h; row++) {
        uint8_t inset = (uint8_t)((row * w) / (2 * h));
        oled_fill_rect(x + inset, y + row, w - 2 * inset, 1, true);
    }
}

static void oled_fill_chevron_up(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    for (uint8_t row = 0; row < h; row++) {
        uint8_t inset = (uint8_t)(((h - 1 - row) * w) / (2 * h));
        oled_fill_rect(x + inset, y + row, w - 2 * inset, 1, true);
    }
}

static void oled_fill_chevron_left(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    for (uint8_t col = 0; col < w; col++) {
        uint8_t inset = (uint8_t)((col * h) / (2 * w));
        oled_fill_rect(x + col, y + inset, 1, h - 2 * inset, true);
    }
}

static void oled_fill_chevron_right(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    for (uint8_t col = 0; col < w; col++) {
        uint8_t inset = (uint8_t)(((w - 1 - col) * h) / (2 * w));
        oled_fill_rect(x + col, y + inset, 1, h - 2 * inset, true);
    }
}

// Icon canvas: full 32px width, top 96px of height (rows 0-11). The remaining
// 4 rows (96-127) are left for the footer text — see hk_oled_render_footer().
#define HK_ICON_H 96
#define HK_CANVAS_H 128

static void hk_oled_draw_gear(uint8_t cx, uint8_t cy, uint8_t r_outer, uint8_t r_inner, uint8_t tooth_size, uint8_t tooth_radius) {
    oled_fill_ring(cx, cy, r_outer, r_inner);
    static const int8_t dirs[8][2] = {{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};
    for (uint8_t i = 0; i < 8; i++) {
        int16_t tx = cx + dirs[i][0] * tooth_radius;
        int16_t ty = cy + dirs[i][1] * tooth_radius;
        oled_fill_rect((uint8_t)(tx - tooth_size / 2), (uint8_t)(ty - tooth_size / 2), tooth_size, tooth_size, true);
    }
}

static void hk_oled_icon_qwerty(void) {
    const uint8_t key = 5, gap = 3, cols = 4, rows = 11;
    const uint8_t ox = (32 - (cols * (key + gap) - gap)) / 2;
    const uint8_t oy = (HK_ICON_H - (rows * (key + gap) - gap)) / 2;
    for (uint8_t r = 0; r < rows; r++) {
        for (uint8_t c = 0; c < cols; c++) {
            oled_fill_rect(ox + c * (key + gap), oy + r * (key + gap), key, key, true);
        }
    }
}

static void hk_oled_icon_lower(void) {
    oled_fill_chevron_down(3, 8, 26, 80);
}

static void hk_oled_icon_raise(void) {
    oled_fill_chevron_up(3, 8, 26, 80);
}

static void hk_oled_icon_adjust(void) {
    const uint8_t cx = 16;
    oled_fill_rect(cx - 1, 22, 2, 44, true);  // connecting shaft
    hk_oled_draw_gear(cx, 14, 8, 5, 3, 10);   // small gear, top
    hk_oled_draw_gear(cx, 80, 13, 9, 4, 14);  // big gear, bottom
}

static void hk_oled_icon_nav(void) {
    oled_fill_chevron_up(11, 2, 10, 30);
    oled_fill_chevron_down(11, 64, 10, 30);
    oled_fill_chevron_left(1, 41, 14, 12);
    oled_fill_chevron_right(17, 41, 14, 12);
}

static void hk_oled_icon_gaming(void) {
    const uint8_t blade_x = 13, blade_w = 6;
    oled_fill_chevron_up(blade_x, 2, blade_w, 10);   // blade tip
    oled_fill_rect(blade_x, 12, blade_w, 48, true);  // blade shaft

    oled_fill_rect(4, 58, 6, 2, true);   // crossguard flare, left
    oled_fill_rect(22, 58, 6, 2, true);  // crossguard flare, right
    oled_fill_rect(4, 60, 24, 3, true);  // crossguard bar

    oled_fill_rect(blade_x, 63, blade_w, 19, true);  // grip
    oled_fill_disc(16, 89, 4);                       // pommel
}

// NOTE: numeric layer order below must match `enum layer_number` in
// keyboards/lily58/keymaps/hk/keymap.c (_QWERTY=0, _LOWER=1, _RAISE=2,
// _ADJUST=3, _NAV=4, _GAMING=5). This file is shared userspace and has no
// visibility into that keymap-local enum, so the mapping is by raw index.
static void hk_oled_render_layer_banner(void) {
    switch (get_highest_layer(layer_state)) {
        case 1:
            hk_oled_icon_lower();
            break;
        case 2:
            hk_oled_icon_raise();
            break;
        case 3:
            hk_oled_icon_adjust();
            break;
        case 4:
            hk_oled_icon_nav();
            break;
        case 5:
            hk_oled_icon_gaming();
            break;
        default:
            hk_oled_icon_qwerty();
            break;
    }
}

static void hk_oled_render_footer(void) {
    // oled_advance_page() wraps using the native (un-rotated) 4-line page
    // count, not our rotated 16-line canvas, so it loops back to the top
    // instead of moving down a row. Use explicit row numbers instead.
    const uint8_t footer_row = HK_ICON_H / 8;

    oled_set_cursor(0, footer_row);
    oled_write_P(pointer_kind_to_string(g_hk_state.main.pointer_kind), false);
#ifdef WPM_ENABLE
    oled_set_cursor(0, footer_row + 1);
    oled_write(format_2d(get_current_wpm()), false);
    oled_write_P(PSTR("wpm"), false);
#endif

    // Modifier indicator row: letter shown inverted (filled background) while held.
    oled_set_cursor(0, footer_row + 2);
    uint8_t mods = get_mods();
    oled_write_char('S', (mods & MOD_MASK_SHIFT) != 0);
    oled_write_char('C', (mods & MOD_MASK_CTRL) != 0);
    oled_write_char('A', (mods & MOD_MASK_ALT) != 0);
    oled_write_char('G', (mods & MOD_MASK_GUI) != 0);
}

#ifndef HK_OLED_IDLE_TIMEOUT_MS
#    define HK_OLED_IDLE_TIMEOUT_MS 15000
#endif

// Slow DVD-logo-style bounce, position derived from elapsed time rather than
// stored state, so there's nothing to reset when the screensaver starts/stops.
static void hk_oled_render_screensaver(void) {
    const uint8_t  size      = 6;
    const uint16_t period_x  = 2 * (32 - size);
    const uint16_t period_y  = 2 * (HK_CANVAS_H - size);
    uint16_t       tx        = (uint16_t)(timer_read32() / 15) % period_x;
    uint16_t       ty        = (uint16_t)(timer_read32() / 23) % period_y;
    uint8_t        x         = tx < period_x / 2 ? tx : (uint8_t)(period_x - tx);
    uint8_t        y         = ty < period_y / 2 ? ty : (uint8_t)(period_y - ty);
    oled_fill_rect(x, y, size, size, true);
}

bool oled_task_user(void) {
    if (g_hk_state.init) {
        oled_clear();
        if (last_input_activity_elapsed() > HK_OLED_IDLE_TIMEOUT_MS) {
            hk_oled_render_screensaver();
        } else {
            hk_oled_render_layer_banner();
            hk_oled_render_footer();
        }
    }
    return true;
}

#else  // !HK_OLED_PORTRAIT_BANNER

bool oled_task_user(void) {
    if (g_hk_state.init) {
        hk_oled_render_keyinfo();
        hk_oled_render_pointer_state();
        hk_oled_render_layerinfo();
    }
    return true;
}

#endif
