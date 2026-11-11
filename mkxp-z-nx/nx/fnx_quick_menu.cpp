/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "fnx_quick_menu.h"
#include "launcher_library.h"
#include <SDL.h>
#include <algorithm>
#include <cmath>

namespace fnx {
namespace {
/* El perfil persiste los 15 botones clásicos. SDL 2 también enumera MISC1,
   paletas y touchpad: SDL_CONTROLLER_BUTTON_MAX NO es la longitud del perfil. */
constexpr int mappedButtons = SDL_CONTROLLER_BUTTON_DPAD_RIGHT + 1;
constexpr int mappedSources = mappedButtons + 2; /* ZL/ZR son ejes, no botones 15/16. */
static_assert(mappedButtons == 15 && SDL_CONTROLLER_BUTTON_A == 0 &&
              SDL_CONTROLLER_BUTTON_GUIDE == 5 &&
              SDL_CONTROLLER_BUTTON_DPAD_RIGHT == 14,
              "El formato del perfil requiere la numeracion SDL 2 esperada");
std::string configRoot, gameId;
SDL_atomic_t active = {}, command = {}, speed = {}, closing = {};
bool held[SDL_CONTROLLER_BUTTON_MAX] = {};
bool triggerHeld[2] = {};
std::string controls = "ABCDEFGHIJKLMNO";
SDL_atomic_t physicalToLogical[mappedSources] = {};
SDL_atomic_t stickFlags = {}, stickDeadzone = {};
SDL_atomic_t capture = {};
SDL_atomic_t exitRequest = {};
bool chordReady = true;
Uint32 chordSince = 0;

bool allReleased() {
    for (bool down : held) if (down) return false;
    for (bool down : triggerHeld) if (down) return false;
    return true;
}
bool chordDown() {
    return held[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] &&
           held[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] &&
           held[SDL_CONTROLLER_BUTTON_START];
}
void post(int value) { SDL_AtomicSet(&command, value); }
void publishControls() {
    for (auto &slot : physicalToLogical) SDL_AtomicSet(&slot, -1);
    for (int logical = 0; logical < mappedButtons; ++logical)
        SDL_AtomicSet(&physicalToLogical[controls[logical] - 'A'], logical);
}
}

void quick_init(const std::string &root, const std::string &game) {
    configRoot = root;
    gameId = game;
    int mode = 0;
    load_game_speed(root, game, mode);
    if (!load_game_buttonmap(root, game, controls)) controls = "ABCDEFGHIJKLMNO";
    publishControls();
    int flags = 0, deadzone = 8000;
    load_game_sticks(root, game, flags, deadzone);
    SDL_AtomicSet(&stickFlags, flags);
    SDL_AtomicSet(&stickDeadzone, deadzone);
    SDL_AtomicSet(&capture, -1);
    SDL_AtomicSet(&exitRequest, 0);
    SDL_AtomicSet(&speed, mode);
    SDL_AtomicSet(&active, 0);
    SDL_AtomicSet(&command, 0);
    SDL_AtomicSet(&closing, 0);
    std::fill(held, held + SDL_CONTROLLER_BUTTON_MAX, false);
    std::fill(triggerHeld, triggerHeld + 2, false);
    chordReady = true;
    chordSince = 0;
}

bool quick_event(const SDL_Event &event) {
    if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        std::fill(held, held + SDL_CONTROLLER_BUTTON_MAX, false);
        std::fill(triggerHeld, triggerHeld + 2, false);
        chordSince = 0;
        chordReady = true;
    }
    const bool pad = event.type == SDL_CONTROLLERBUTTONDOWN ||
                     event.type == SDL_CONTROLLERBUTTONUP;
    if (pad && event.cbutton.button < SDL_CONTROLLER_BUTTON_MAX)
        held[event.cbutton.button] = event.type == SDL_CONTROLLERBUTTONDOWN;
    if (pad && !chordDown()) {
        chordSince = 0;
        if (!SDL_AtomicGet(&active)) chordReady = true;
    }
    if (event.type == SDL_CONTROLLERAXISMOTION &&
        event.caxis.axis >= SDL_CONTROLLER_AXIS_TRIGGERLEFT &&
        event.caxis.axis <= SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
        const int trigger = event.caxis.axis - SDL_CONTROLLER_AXIS_TRIGGERLEFT;
        const bool pressed = event.caxis.value >= quick_trigger_threshold();
        if (SDL_AtomicGet(&active) && SDL_AtomicGet(&capture) >= 0 &&
            pressed && !triggerHeld[trigger]) {
            post(QuickCaptureBase + 15 + trigger);
            SDL_AtomicSet(&capture, -1);
        }
        triggerHeld[trigger] = pressed;
    }
    if (SDL_AtomicGet(&closing)) {
        if (allReleased()) SDL_AtomicSet(&closing, 0);
        return pad || event.type == SDL_KEYDOWN || event.type == SDL_KEYUP ||
               event.type == SDL_CONTROLLERAXISMOTION;
    }
    if (SDL_AtomicGet(&active)) {
        if (event.type == SDL_CONTROLLERBUTTONDOWN) {
            if (SDL_AtomicGet(&capture) >= 0) {
                if (event.cbutton.button < mappedButtons &&
                    event.cbutton.button != SDL_CONTROLLER_BUTTON_GUIDE) {
                    post(QuickCaptureBase + event.cbutton.button);
                    SDL_AtomicSet(&capture, -1);
                }
                return true;
            }
            switch (event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_UP: post(QuickUp); break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN: post(QuickDown); break;
                case SDL_CONTROLLER_BUTTON_B: post(QuickSelect); break;
                case SDL_CONTROLLER_BUTTON_A: post(QuickBack); break;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: post(QuickPreviousPage); break;
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: post(QuickNextPage); break;
                default: break;
            }
        } else if (event.type == SDL_KEYDOWN && !event.key.repeat) {
            if (SDL_AtomicGet(&capture) >= 0 && event.key.keysym.sym == SDLK_ESCAPE) {
                post(QuickBack);
                SDL_AtomicSet(&capture, -1);
                return true;
            }
            switch (event.key.keysym.sym) {
                case SDLK_UP: post(QuickUp); break;
                case SDLK_DOWN: post(QuickDown); break;
                case SDLK_RETURN: post(QuickSelect); break;
                case SDLK_ESCAPE: post(QuickBack); break;
                default: break;
            }
        }
        return pad || event.type == SDL_KEYDOWN || event.type == SDL_KEYUP ||
               event.type == SDL_CONTROLLERAXISMOTION ||
               event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP ||
               event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEWHEEL ||
               event.type == SDL_TEXTINPUT ||
               event.type == SDL_FINGERDOWN || event.type == SDL_FINGERUP ||
               event.type == SDL_FINGERMOTION;
    }
    /* El segundo hombro y PLUS quedan reservados para el atajo. El primero
       puede haber llegado al juego durante el inicio de la combinación. */
    return pad && quick_chord_candidate() &&
           (event.cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER ||
            event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER ||
            event.cbutton.button == SDL_CONTROLLER_BUTTON_START);
}

bool quick_tick() {
    if (SDL_AtomicGet(&closing) && allReleased()) SDL_AtomicSet(&closing, 0);
    if (SDL_AtomicGet(&active) || !chordReady || !chordDown()) return false;
    const Uint32 now = SDL_GetTicks();
    if (!chordSince) chordSince = now ? now : 1;
    if (now - chordSince < 800) return false;
    chordReady = false;
    SDL_AtomicSet(&active, 1);
    SDL_AtomicSet(&command, 0);
    return true;
}
bool quick_active() { return SDL_AtomicGet(&active) != 0; }
bool quick_chord_candidate() {
    return held[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] &&
           held[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER];
}
int quick_take_command() { return SDL_AtomicSet(&command, 0); }
void quick_close() {
    SDL_AtomicSet(&active, 0);
    SDL_AtomicSet(&command, 0);
    SDL_AtomicSet(&closing, 1);
    SDL_AtomicSet(&capture, -1);
}
int quick_speed() { return SDL_AtomicGet(&speed); }
bool quick_set_speed(int mode) {
    if (mode < 0 || mode > 4 || !save_game_speed(configRoot, gameId, mode)) return false;
    SDL_AtomicSet(&speed, mode);
    return true;
}
const char *quick_speed_label(int mode) {
    static const char *labels[] = {"Automatico", "0,5x", "1x", "1,5x", "2x"};
    return mode >= 0 && mode <= 4 ? labels[mode] : labels[0];
}
double quick_multiplier(int mode) {
    static const double values[] = {1.0, 0.5, 1.0, 1.5, 2.0};
    return mode >= 0 && mode <= 4 ? values[mode] : 1.0;
}
int quick_rate(int nominal_hz, int mode) {
    return std::max(1, (int)std::lround(nominal_hz * quick_multiplier(mode)));
}
int quick_game_button(int physical) {
    return physical >= 0 && physical < mappedButtons ?
           SDL_AtomicGet(&physicalToLogical[physical]) : physical;
}
int quick_game_trigger_button(int axis) {
    if (axis < SDL_CONTROLLER_AXIS_TRIGGERLEFT || axis > SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
        return -1;
    return SDL_AtomicGet(&physicalToLogical[mappedButtons + axis - SDL_CONTROLLER_AXIS_TRIGGERLEFT]);
}
int quick_trigger_threshold() { return 16000; }
const char *quick_controls() { return controls.c_str(); }
const char *quick_button_name(int button) {
    static const char *names[] = {"A", "B", "X", "Y", "MINUS", "GUIDE",
        "PLUS", "L3", "R3", "L", "R", "ARRIBA", "ABAJO", "IZQUIERDA", "DERECHA",
        "ZL", "ZR"};
    return button >= 0 && button < 17 ? names[button] : "?";
}
int quick_button_source(int logical) {
    return logical >= 0 && logical < mappedButtons ?
           controls[logical] - 'A' : -1;
}
int quick_stick_flags() { return SDL_AtomicGet(&stickFlags); }
int quick_stick_deadzone() { return SDL_AtomicGet(&stickDeadzone); }
bool quick_set_sticks(int flags, int deadzone) {
    if (!save_game_sticks(configRoot, gameId, flags, deadzone)) return false;
    SDL_AtomicSet(&stickFlags, flags);
    SDL_AtomicSet(&stickDeadzone, deadzone);
    return true;
}
int quick_game_axis(int physical, int value, int &logical) {
    logical = physical;
    if (physical < 0 || physical >= SDL_CONTROLLER_AXIS_MAX) return value;
    if (physical >= SDL_CONTROLLER_AXIS_TRIGGERLEFT) return value;
    const int flags = quick_stick_flags();
    if (flags & 1) logical = (physical + 2) % 4;
    const int invertBit = 1 << (logical + 1);
    if (flags & invertBit) value = value == -32768 ? 32767 : -value;
    if (std::abs(value) < quick_stick_deadzone()) value = 0;
    return value;
}
void quick_project_controller(const bool *physicalButtons, const int *physicalAxes,
                              bool *logicalButtons, int *logicalAxes) {
    std::fill(logicalButtons, logicalButtons + SDL_CONTROLLER_BUTTON_MAX, false);
    std::fill(logicalAxes, logicalAxes + SDL_CONTROLLER_AXIS_MAX, 0);
    for (int source = 0; source < SDL_CONTROLLER_BUTTON_MAX; ++source) {
        if (!physicalButtons[source]) continue;
        const int target = quick_game_button(source);
        if (target >= 0 && target < SDL_CONTROLLER_BUTTON_MAX)
            logicalButtons[target] = true;
    }
    for (int source = 0; source < SDL_CONTROLLER_AXIS_MAX; ++source) {
        int target = source;
        const int value = quick_game_axis(source, physicalAxes[source], target);
        if (target >= 0 && target < SDL_CONTROLLER_AXIS_MAX)
            logicalAxes[target] = value;
        const int button = quick_game_trigger_button(source);
        if (button >= 0 && button < SDL_CONTROLLER_BUTTON_MAX &&
            physicalAxes[source] >= quick_trigger_threshold())
            logicalButtons[button] = true;
    }
}
void quick_capture_start(int logical) { SDL_AtomicSet(&capture, logical); }
void quick_capture_cancel() { SDL_AtomicSet(&capture, -1); }
bool quick_assign(int logical, int physical) {
    if (logical < 0 || logical >= mappedButtons || logical == SDL_CONTROLLER_BUTTON_GUIDE ||
        physical < 0 || physical >= mappedSources || physical == SDL_CONTROLLER_BUTTON_GUIDE) return false;
    std::string next = controls;
    const size_t previous = next.find((char)('A' + physical));
    if (previous < mappedButtons)
        std::swap(next[logical], next[previous]);
    else next[logical] = (char)('A' + physical);
    if (!save_game_buttonmap(configRoot, gameId, next)) return false;
    controls = next;
    publishControls();
    return true;
}
bool quick_reset_controls() {
    if (!save_game_buttonmap(configRoot, gameId, "ABCDEFGHIJKLMNO") ||
        !save_game_sticks(configRoot, gameId, 0, 8000)) return false;
    controls = "ABCDEFGHIJKLMNO";
    publishControls();
    SDL_AtomicSet(&stickFlags, 0);
    SDL_AtomicSet(&stickDeadzone, 8000);
    return true;
}
void quick_request_exit() { SDL_AtomicSet(&exitRequest, 1); }
bool quick_take_exit_request() { return SDL_AtomicSet(&exitRequest, 0) != 0; }
bool quick_exit_requested() { return SDL_AtomicGet(&exitRequest) != 0; }
int quick_menu_row_count(int view) {
    switch (view) {
        case 0: case 1: return 5;  /* principal, velocidad */
        case 3: return 2;          /* confirmación de salida */
        case 4: return 8;          /* cinco entradas, página, restaurar, volver */
        case 6: return 8;          /* opciones de sticks y restauración */
        case 2: case 5: return 1;  /* diagnóstico, captura */
        default: return 0;
    }
}
const char *quick_menu_row_label(int view, int index) {
    static const char *mainRows[] = {"Reanudar juego", "Velocidad",
        "Controles del juego", "Diagnostico", "Volver a la biblioteca"};
    static const char *speedRows[] = {"Automatico / nominal", "0,5x", "1x", "1,5x", "2x"};
    static const char *confirmRows[] = {"Cancelar", "Salir sin guardar el progreso reciente"};
    if (index < 0 || index >= quick_menu_row_count(view)) return nullptr;
    switch (view) {
        case 0: return mainRows[index];
        case 1: return speedRows[index];
        case 3: return confirmRows[index];
        case 4: return index < 5 ? "Entrada del juego" :
                       index == 5 ? "Pagina siguiente" :
                       index == 6 ? "Opciones de sticks" : "Volver";
        case 6: {
            static const char *sticks[] = {"Intercambiar sticks L / R", "Invertir L X",
                "Invertir L Y", "Invertir R X", "Invertir R Y", "Zona muerta",
                "Restaurar controles predeterminados", "Volver"};
            return sticks[index];
        }
        default: return nullptr;
    }
}
}
