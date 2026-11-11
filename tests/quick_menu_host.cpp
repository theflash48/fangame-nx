/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "fnx_quick_menu.h"
#include "launcher_library.h"
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
static SDL_Event axis(int code,int value) {
    SDL_Event e{}; e.type=SDL_CONTROLLERAXISMOTION; e.caxis.axis=(unsigned char)code;
    e.caxis.value=value; return e;
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
    assert(std::string(fnx::quick_controls())=="ABCDEFGHIJKLMNO");
    assert(fnx::quick_assign(0,SDL_CONTROLLER_BUTTON_Y));
    assert(std::string(fnx::quick_controls())=="DBCAEFGHIJKLMNO");
    assert(fnx::quick_game_button(SDL_CONTROLLER_BUTTON_Y)==SDL_CONTROLLER_BUTTON_A);
    assert(fnx::quick_game_button(SDL_CONTROLLER_BUTTON_A)==SDL_CONTROLLER_BUTTON_Y);
    fnx::quick_init(root,"pokemon-opalo");
    assert(std::string(fnx::quick_controls())=="DBCAEFGHIJKLMNO");
    fnx::quick_init(root,"pokemon-z"); assert(fnx::quick_speed()==0);
    assert(std::string(fnx::quick_controls())=="ABCDEFGHIJKLMNO");
    assert(fnx::quick_set_speed(1) && fnx::quick_multiplier(fnx::quick_speed())==0.5);
    fnx::quick_init(root,"pokemon-opalo"); assert(fnx::quick_speed()==3);
    assert(std::string(fnx::quick_controls())=="DBCAEFGHIJKLMNO");
    assert(fnx::quick_reset_controls());
    assert(std::string(fnx::quick_controls())=="ABCDEFGHIJKLMNO");
    assert(!fnx::quick_assign(0,17));
    assert(!fnx::quick_assign(SDL_CONTROLLER_BUTTON_GUIDE,0));
    assert(fnx::quick_assign(SDL_CONTROLLER_BUTTON_DPAD_UP,SDL_CONTROLLER_BUTTON_RIGHTSTICK));
    assert(fnx::quick_game_button(SDL_CONTROLLER_BUTTON_RIGHTSTICK)==SDL_CONTROLLER_BUTTON_DPAD_UP);
    assert(fnx::quick_game_button(SDL_CONTROLLER_BUTTON_DPAD_UP)==SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    assert(fnx::quick_assign(SDL_CONTROLLER_BUTTON_A,15));
    assert(fnx::quick_game_trigger_button(SDL_CONTROLLER_AXIS_TRIGGERLEFT)==SDL_CONTROLLER_BUTTON_A);
    assert(fnx::quick_game_button(SDL_CONTROLLER_BUTTON_A)==-1);
    fnx::quick_init(root,"pokemon-opalo");
    assert(fnx::quick_game_trigger_button(SDL_CONTROLLER_AXIS_TRIGGERLEFT)==SDL_CONTROLLER_BUTTON_A);
    assert(fnx::quick_reset_controls());
    assert(fnx::quick_set_sticks(1|2|16,12000));
    int logical=-1;
    assert(fnx::quick_game_axis(SDL_CONTROLLER_AXIS_RIGHTX,10000,logical)==0 && logical==SDL_CONTROLLER_AXIS_LEFTX);
    assert(fnx::quick_game_axis(SDL_CONTROLLER_AXIS_RIGHTX,20000,logical)==-20000 && logical==SDL_CONTROLLER_AXIS_LEFTX);
    assert(fnx::quick_game_axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT,20000,logical)==20000 && logical==SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    fnx::quick_init(root,"pokemon-opalo");
    assert(fnx::quick_stick_flags()==19 && fnx::quick_stick_deadzone()==12000);
    assert(fnx::quick_reset_controls());
    assert(fnx::save_game_controls(root,"legacy","YBXA"));
    fnx::quick_init(root,"legacy");
    assert(std::string(fnx::quick_controls())=="DBCAEFGHIJKLMNO");
    fnx::quick_init(root,"pokemon-opalo");
    fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_LEFTSHOULDER));
    fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER));
    fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_START));
    fake_ticks=1200; assert(!fnx::quick_tick());
    fake_ticks=2001; assert(fnx::quick_tick());
    fnx::quick_capture_start(0);
    assert(fnx::quick_event(axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,20000)));
    assert(fnx::quick_take_command()==fnx::QuickCaptureBase+16);
    fnx::quick_capture_start(0);
    assert(fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_Y)));
    assert(fnx::quick_take_command()==fnx::QuickCaptureBase+SDL_CONTROLLER_BUTTON_Y);
    assert(fnx::quick_assign(0,SDL_CONTROLLER_BUTTON_Y));
    assert(fnx::quick_game_button(SDL_CONTROLLER_BUTTON_Y)==SDL_CONTROLLER_BUTTON_A);
    fnx::quick_close();
    for(int code : {SDL_CONTROLLER_BUTTON_Y,SDL_CONTROLLER_BUTTON_START,
                    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER})
        fnx::quick_event(button(SDL_CONTROLLERBUTTONUP,code));
    fnx::quick_init(root,"pokemon-opalo");
    /* Reproduce la transición que en b35 dibujaba cinco filas en una vista
       de confirmación con solo dos cadenas definidas. */
    int view = 0;
    assert(fnx::quick_menu_row_count(view) == 5);
    view = 3;
    assert(fnx::quick_menu_row_count(view) == 2);
    for (int testView : {0, 1, 3})
        for (int i = 0; i < fnx::quick_menu_row_count(testView); ++i)
            assert(fnx::quick_menu_row_label(testView, i) != nullptr);
    assert(fnx::quick_menu_row_label(view, 2) == nullptr);
    assert(fnx::quick_menu_row_count(4) == 8);
    for (int testView : {4, 6})
        for (int i = 0; i < fnx::quick_menu_row_count(testView); ++i)
            assert(fnx::quick_menu_row_label(testView, i) != nullptr);
    assert(fnx::quick_menu_row_count(9) == 0);
    assert(!fnx::quick_set_speed(9));
    assert(!fnx::quick_exit_requested());
    fnx::quick_request_exit();
    assert(fnx::quick_exit_requested());
    assert(fnx::quick_take_exit_request());
    assert(!fnx::quick_take_exit_request());
    fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_LEFTSHOULDER));
    fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER));
    fnx::quick_event(button(SDL_CONTROLLERBUTTONDOWN,SDL_CONTROLLER_BUTTON_START));
    fake_ticks=1000; assert(!fnx::quick_tick());
    fake_ticks=1100; fnx::quick_event(button(SDL_CONTROLLERBUTTONUP,SDL_CONTROLLER_BUTTON_START));
    assert(!fnx::quick_tick() && !fnx::quick_active());
    std::cout<<"quick_menu_host: OK\n";
}
