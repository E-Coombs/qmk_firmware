# Fixed Issue: Intermittent Trackpad X/Y Axis Inversion

## Symptom

The Azoteq TPS43 trackpad (right half) occasionally has its X/Y axes inverted —
moving right registers as left, moving up registers as down. The inversion is
intermittent: sometimes correct, sometimes flipped, with no user action that
reliably reproduces it.

## Root Cause

Two independently reasonable pieces of code interact badly:

1. **Toggle-based axis config.** `azoteq_iqs5xx_set_xy_config()`
   (`qmk_firmware/drivers/sensors/azoteq_iqs5xx.c:227-247`) sets `flip_x`/`flip_y`
   by XOR-ing against whatever the chip currently reports
   (`config.flip_x = !config.flip_x`), not by writing an absolute `true`/`false`.
   This only produces a consistent result if the chip's flip bits start from a
   known state every time it runs.

   `azoteq_iqs5xx_init()` (same file, line 324) calls this once at boot. For
   this build, `AZOTEQ_IQS5XX_ROTATION_180` is defined
   (`qmk_firmware/users/holykeebs/config.h`), so init flips both X and Y via:
   ```c
   azoteq_iqs5xx_init_status |= azoteq_iqs5xx_set_xy_config(true, true, false, true, false);
   ```
   A `reset` is issued on `SYSTEM_CONTROL_1` right before this
   (`azoteq_iqs5xx_reset_suspend(true, false, true)`, line 327), which the
   toggle logic implicitly assumes reloads `XY_CONFIG_0` (0x0669, where
   flip_x/flip_y live) back to factory defaults. This assumption is not
   documented anywhere — not in driver comments, not in the original upstream
   commit (`68722d35a3`, "Azoteq IQS5xx support #22280") — it's inferred purely
   from the code structure. In practice the "reset" bit is most likely a *soft*
   reset of the touch/finger state machine and does **not** reload persistent
   config registers like `XY_CONFIG_0`. So the flip bits persist across calls
   to `init()`.

2. **Split watchdog reset.** `users/holykeebs/config.h` defines
   `SPLIT_WATCHDOG_ENABLE` / `SPLIT_WATCHDOG_TIMEOUT 3000`, added so the
   keyboard is detected correctly if already plugged in at boot.
   `split_watchdog_task()` (`qmk_firmware/quantum/split_common/split_util.c`)
   calls `mcu_reset()` on the right half if the split link isn't established
   within 3 seconds — this runs continuously as part of `keyboard_task()`, not
   just once at boot.

   An MCU reset re-executes `keyboard_init()` → `pointing_device_init()` →
   `azoteq_iqs5xx_init()`. The IQS5xx chip itself is never power-cycled by this
   (it's I2C-connected, independent power domain from the MCU), so its
   register state — including the already-toggled flip bits — survives. Each
   unintended re-init toggles the bits again, flipping them back.

**Net effect:** whether the trackpad reads correctly or inverted depends on the
parity of how many times the right-half MCU has reset since the IQS5xx chip's
last real power cycle. Any transient split-link hiccup (USB hub timing, brief
serial noise, slave boot race) silently flips the axes for the rest of that
session.

## Fix (applied)

`azoteq_iqs5xx_set_xy_config()` now writes absolute values instead of
toggling against whatever the chip currently reports. The existing call sites
in `azoteq_iqs5xx_init()` already passed the correct absolute desired state
per rotation case, so no changes were needed there — only the toggle logic
itself was wrong. Repeated calls to `init()` are now idempotent regardless of
how many times the right-half MCU has reset since power-on.

## Related Issue: Trackpad Sometimes Not Enabled on Plug-In

Same underlying trigger (split watchdog causing repeated unintended re-inits)
also exposed a second bug: `azoteq_iqs5xx_init()` checked
`azoteq_iqs5xx_get_product()` exactly once, with no retry. If one of those
re-inits caught the chip mid-settle (e.g. right after the MCU itself just
reset and the I2C peripheral was reinitializing), the product check returned
`AZOTEQ_IQS5XX_UNKNOWN`, the whole config block was skipped, and
`azoteq_iqs5xx_init_status` was left in its failed default state for the rest
of the session — `azoteq_iqs5xx_get_report()` bails out immediately whenever
`init_status != I2C_STATUS_SUCCESS`, so the trackpad stayed dead until the
next full power cycle.

### Fix (applied)

`azoteq_iqs5xx_init()` now retries the product check up to
`AZOTEQ_IQS5XX_INIT_RETRIES` (default 5) times, waiting
`AZOTEQ_IQS5XX_INIT_RETRY_DELAY_MS` (default 50ms) between attempts and
re-pinging the I2C address before each retry, before giving up.
