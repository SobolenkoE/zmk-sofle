/*
 * Behavior "enc_wheel": мгновенные HID wheel-события для энкодера.
 *
 * Обход бага двухосевых behaviors (&msc/&mmv) на ZMK main: при мгновенном
 * press/release (как у sensor-rotate) tick_work отменяется раньше первого
 * срабатывания, и wheel-события до listener'а не доезжают (см. логи:
 * update_movement тикает, scroll_set всегда 0/0). Здесь колесо шлём
 * напрямую: press → mouse_scroll_update + send_mouse_report.
 *
 * Параметр binding'а — число шагов за тик, отрицательное = прокрутка вниз:
 *   &enc_wheel 5   (вверх)   /   &enc_wheel (-5)   (вниз)
 */

#define DT_DRV_COMPAT zmk_behavior_enc_wheel

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    int8_t steps = (int8_t)binding->param1;
    if (steps == 0) {
        return ZMK_BEHAVIOR_OPAQUE;
    }

/* hid.c/endpoints.c собираются только на центральной половине
 * ((NOT ZMK_SPLIT) OR ZMK_SPLIT_ROLE_CENTRAL, см. app/CMakeLists.txt).
 * Энкодер и так физически на центральной — на периферии вызов не случится. */
#if IS_ENABLED(CONFIG_ZMK_POINTING) &&                                                              \
    (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
    LOG_DBG("encoder wheel: %d", steps);
    zmk_hid_mouse_scroll_update(0, steps);
    return zmk_endpoint_send_mouse_report();
#else
    return ZMK_BEHAVIOR_OPAQUE;
#endif /* CONFIG_ZMK_POINTING && (не split || central) */
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_enc_wheel_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif /* IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA) */
};

#define ENC_WHEEL_INST(n)                                                                          \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_enc_wheel_driver_api)

DT_INST_FOREACH_STATUS_OKAY(ENC_WHEEL_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
