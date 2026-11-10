/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FNX_QUICK_MENU_H
#define FNX_QUICK_MENU_H
#include <string>
union SDL_Event;
namespace fnx {
enum QuickCommand { QuickNone=0, QuickUp, QuickDown, QuickSelect, QuickBack,
                    QuickCaptureBase=100 };
void quick_init(const std::string &root, const std::string &game);
/* Solo el hilo de eventos llama estas dos funciones. */
bool quick_event(const SDL_Event &event);
bool quick_tick();
bool quick_chord_candidate();
/* Solo el hilo RGSS usa los comandos y dibuja el menú. */
bool quick_active();
int quick_take_command();
void quick_close();
int quick_speed();
bool quick_set_speed(int mode);
const char *quick_speed_label(int mode);
double quick_multiplier(int mode);
int quick_rate(int nominal_hz, int mode);
int quick_game_button(int physical);
const char *quick_controls();
void quick_capture_start(int logical);
void quick_capture_cancel();
bool quick_assign(int logical, int physical);
bool quick_reset_controls();
/* Solicitud de salida que solo el binding Ruby atiende con el GVL adquirido. */
void quick_request_exit();
bool quick_take_exit_request();
bool quick_exit_requested();
}
#endif
