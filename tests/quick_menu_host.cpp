/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "fnx_quick_menu.h"
#include "fnx_frame_pacer.h"
#include <SDL.h>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
Uint32 fake_ticks=0;
static SDL_Event button(int type,int code) {
    SDL_Event e{}; e.type=type; e.cbutton.button=(unsigned char)code; return e;
}
int main(int argc,char **argv) {
    assert(argc==2);
    const std::string root=(std::filesystem::path(argv[1]) /
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())).string();
    fnx::FrameDeadline pace;
    assert(pace.advance(1000,25)==1025);
    assert(pace.advance(1033,25)==1050); /* VSync de 60 Hz tras 40 Hz nominal */
    assert(pace.advance(1052,25)==1075);
    assert(pace.advance(1500,25)==1525); /* sin ráfaga tras pausa */
    pace.reset(); assert(pace.advance(2000,50)==2050); /* 0,5x */
    std::filesystem::create_directories(std::filesystem::path(root)/"config");
    fnx::quick_init(root,"pokemon-opalo");
    assert(fnx::quick_speed()==0 && fnx::quick_multiplier(0)==1.0);
    assert(fnx::quick_rate(40,0)==40 && fnx::quick_rate(40,3)==60 &&
           fnx::quick_rate(40,4)==80 && fnx::quick_rate(60,1)==30);
    assert(!fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_B)));
    fnx::quick_event(button(SDL_CONTROLLERBUTTONUP,SDL_CONTROLLER_BUTTON_B));
    fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_LEFTSHOULDER));
    assert(fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)));
    assert(fnx::quick_chord_candidate());
    assert(fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_START)));
    fake_ticks=100; assert(!fnx::quick_tick());
    fake_ticks=899; assert(!fnx::quick_tick());
    fake_ticks=901; assert(fnx::quick_tick() && fnx::quick_active());
    assert(!fnx::quick_tick());
    assert(fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_DPAD_DOWN)));
    assert(fnx::quick_take_command()==fnx::QuickDown);
    assert(fnx::quick_take_command()==fnx::QuickNone);
    assert(fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_B)));
    assert(fnx::quick_take_command()==fnx::QuickSelect);
    assert(fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_A)));
    assert(fnx::quick_take_command()==fnx::QuickBack);
    assert(fnx::quick_set_speed(3) && fnx::quick_multiplier(fnx::quick_speed())==1.5);
    fnx::quick_close();
    assert(!fnx::quick_active());
    for(int code : {SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_B,
                    SDL_CONTROLLER_BUTTON_DPAD_DOWN,SDL_CONTROLLER_BUTTON_START,
                    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER})
        fnx::quick_event(button(SDL_CONTROLLERBUTTONUP,code));
    fnx::quick_init(root,"pokemon-opalo"); assert(fnx::quick_speed()==3);
    fnx::quick_init(root,"pokemon-z"); assert(fnx::quick_speed()==0);
    assert(fnx::quick_set_speed(1) && fnx::quick_multiplier(fnx::quick_speed())==0.5);
    fnx::quick_init(root,"pokemon-opalo"); assert(fnx::quick_speed()==3);
    assert(!fnx::quick_set_speed(9));
    fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_LEFTSHOULDER));
    fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER));
    fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_START));
    fake_ticks=1000; assert(!fnx::quick_tick());
    fake_ticks=1100; fnx::quick_event(button(SDL_CONTROLLERBUTTONUP,SDL_CONTROLLER_BUTTON_START));
    assert(!fnx::quick_tick() && !fnx::quick_active());
    std::cout<<"quick_menu_host: OK\n";
}
