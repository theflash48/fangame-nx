/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "fnx_quick_menu.h"
#include "launcher_library.h"
#include <SDL.h>
#include <algorithm>
#include <cmath>

namespace fnx {
namespace {
std::string configRoot, gameId;
SDL_atomic_t active = {}, command = {}, speed = {}, closing = {};
bool held[SDL_CONTROLLER_BUTTON_MAX] = {};
bool chordReady = true;
Uint32 chordSince = 0;

bool allReleased() {
    for (bool down : held) if (down) return false;
    return true;
}
bool chordDown() {
    return held[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] &&
           held[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] &&
           held[SDL_CONTROLLER_BUTTON_START];
}
void post(int value) { SDL_AtomicSet(&command, value); }
}

void quick_init(const std::string &root, const std::string &game) {
    configRoot = root;
    gameId = game;
    int mode = 0;
    load_game_speed(root, game, mode);
    SDL_AtomicSet(&speed, mode);
    SDL_AtomicSet(&active, 0);
    SDL_AtomicSet(&command, 0);
    SDL_AtomicSet(&closing, 0);
    std::fill(held, held + SDL_CONTROLLER_BUTTON_MAX, false);
    chordReady = true;
    chordSince = 0;
}

bool quick_event(const SDL_Event &event) {
    if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        std::fill(held, held + SDL_CONTROLLER_BUTTON_MAX, false);
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
    if (SDL_AtomicGet(&closing)) {
        if (allReleased()) SDL_AtomicSet(&closing, 0);
        return pad || event.type == SDL_KEYDOWN || event.type == SDL_KEYUP;
    }
    if (SDL_AtomicGet(&active)) {
        if (event.type == SDL_CONTROLLERBUTTONDOWN) {
            switch (event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_UP: post(QuickUp); break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN: post(QuickDown); break;
                case SDL_CONTROLLER_BUTTON_B: post(QuickSelect); break;
                case SDL_CONTROLLER_BUTTON_A: post(QuickBack); break;
                default: break;
            }
        } else if (event.type == SDL_KEYDOWN && !event.key.repeat) {
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
}
