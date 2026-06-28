/*
This is the c configuration file for the keymap

Copyright 2012 Jun Wako <wakojun@gmail.com>
Copyright 2015 Jack Humbert

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

//#define USE_MATRIX_I2C

/* Select hand configuration */

#define MASTER_LEFT
// #define MASTER_RIGHT
// #define EE_HANDS

#define QUICK_TAP_TERM 0
#define TAPPING_TERM 200

#define DYNAMIC_KEYMAP_LAYER_COUNT 6

#define COMBO_COUNT 1

// OLED is mounted in portrait; opt in to the icon-based portrait renderer in
// users/holykeebs/oled.c instead of its default landscape debug view.
#define HK_OLED_PORTRAIT_BANNER

// The portrait OLED's modifier indicator calls get_mods() directly. Only the
// master half tracks live mod state by default — this syncs it to the slave
// half too, so the indicator works correctly on both screens.
#define SPLIT_MODS_ENABLE

// users/holykeebs/config.h defaults the right-side TPS43 to AZOTEQ_IQS5XX_ROTATION_180, but on
// this unit that left down/left registering as right/up. The fix needs the 180-degree flip plus
// an axis swap (see drivers/sensors/azoteq_iqs5xx.c), which isn't one of the standard presets.
#undef AZOTEQ_IQS5XX_ROTATION_180
#define AZOTEQ_IQS5XX_ROTATION_180_MIRRORED

// Underglow
/*
#undef RGBLIGHT_LED_COUNT
#define RGBLIGHT_LED_COUNT 14    // Number of LEDs
#define RGBLIGHT_SLEEP
*/
