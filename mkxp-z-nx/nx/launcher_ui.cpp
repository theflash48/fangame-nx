/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "launcher_ui.h"
#include "launcher_library.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <algorithm>
#include <cstdio>
#include <sys/stat.h>

extern unsigned char ___assets_liberation_ttf[];
extern unsigned int ___assets_liberation_ttf_len;

namespace fnx {
static const SDL_Color bg = {12, 20, 35, 255};
static const SDL_Color panel = {23, 35, 54, 255};
static const SDL_Color inset = {31, 47, 68, 255};
static const SDL_Color white = {237, 245, 252, 255};
static const SDL_Color muted = {157, 178, 198, 255};
static const SDL_Color accent = {66, 221, 207, 255};
static const SDL_Color warning = {255, 183, 115, 255};

static void log_line(const std::string &root, const char *line) {
    mkdir((root + "/logs").c_str(), 0755);
    FILE *f = fopen((root + "/logs/launcher.log").c_str(), "ab");
    if (f) { fprintf(f, "%s\n", line); fclose(f); }
}
static void box(SDL_Renderer *r, int x, int y, int w, int h, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 255);
    SDL_Rect rect = {x,y,w,h};
    SDL_RenderFillRect(r, &rect);
}
static void label(SDL_Renderer *r, TTF_Font *font, int x, int y,
                  const std::string &s, SDL_Color color, int maxw = 1100) {
    if (s.empty()) return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, s.c_str(), color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    if (tex) {
        SDL_Rect dest = {x,y,std::min(surf->w,maxw),surf->h};
        SDL_Rect src = {0,0,dest.w,surf->h};
        SDL_RenderCopy(r, tex, &src, &dest);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}
static void cover(SDL_Renderer *r, int x, int y, int w, int h, unsigned seed) {
    box(r,x,y,w,h,inset);
    SDL_Color c1 = {(Uint8)(43 + seed%40), (Uint8)(105 + seed%50), 150, 255};
    SDL_Color c2 = {43, (Uint8)(150 + seed%50), (Uint8)(175 + seed%60), 255};
    box(r,x+16,y+16,w-32,h-32,c1);
    box(r,x+32,y+32,w-64,h-64,panel);
    box(r,x+32,y+h/2-5,w-64,10,c2);
    box(r,x+w/2-5,y+32,10,h-64,c2);
}
static void paragraph(SDL_Renderer *r, TTF_Font *font, int x, int y,
                      const std::string &s, SDL_Color color, size_t width, int lines) {
    size_t pos = 0;
    while (pos < s.size() && lines-- > 0) {
        size_t end = std::min(s.size(), pos + width);
        if (end < s.size()) {
            size_t space = s.rfind(' ', end);
            if (space != std::string::npos && space > pos) end = space;
        }
        label(r,font,x,y,s.substr(pos,end-pos),color);
        y += TTF_FontHeight(font) + 8;
        pos = end;
        while (pos < s.size() && s[pos] == ' ') ++pos;
    }
}
enum View { LIBRARY, DETAILS, SETTINGS, MESSAGE };
struct Ui {
    std::string root, message;
    Scan scan;
    View view = LIBRARY;
    View message_back = LIBRARY;
    size_t selected = 0;
    int settings = 0;
    bool compact = false;
    bool running = true;
    std::string launch;
    std::string cover_path;
    SDL_Texture *cover_texture = nullptr;
};
static void game_cover(SDL_Renderer *r, Ui &u, const Game &g, int x, int y, int w, int h) {
    if (u.cover_path != g.cover) {
        if (u.cover_texture) SDL_DestroyTexture(u.cover_texture);
        u.cover_texture=nullptr;
        u.cover_path=g.cover;
        if (!g.cover.empty()) {
            SDL_Surface *s=IMG_Load(g.cover.c_str());
            if (s) {
                if (s->w<=2048 && s->h<=2048) u.cover_texture=SDL_CreateTextureFromSurface(r,s);
                SDL_FreeSurface(s);
            }
        }
    }
    if (!u.cover_texture) { cover(r,x,y,w,h,(unsigned)u.selected*17); return; }
    box(r,x,y,w,h,inset);
    int tw=0,th=0;
    SDL_QueryTexture(u.cover_texture,nullptr,nullptr,&tw,&th);
    if (tw<=0 || th<=0) return;
    float scale=std::min((float)w/tw,(float)h/th);
    SDL_Rect dest={x+(w-(int)(tw*scale))/2,y+(h-(int)(th*scale))/2,
                   (int)(tw*scale),(int)(th*scale)};
    SDL_RenderCopy(r,u.cover_texture,nullptr,&dest);
}
static void rescan(Ui &u) {
    u.scan = scan(u.root);
    if (u.selected >= u.scan.games.size()) u.selected = 0;
    log_line(u.root, u.scan.error.empty() ? "scan: terminado" : "scan: error de lectura");
}
static void render(SDL_Renderer *r, TTF_Font *title, TTF_Font *normal, TTF_Font *small, Ui &u) {
    box(r,0,0,1280,720,bg);
    box(r,0,0,1280,8,accent);
    label(r,title,52,35,"FangameNX",white);
    label(r,small,1000,48,"LAUNCHER  0.1",muted,250);
    box(r,50,104,1180,1,inset);
    if (u.view == LIBRARY) {
        label(r,normal,52,126,"Biblioteca",white);
        label(r,small,52,164,"Juegos preparados: " + std::to_string(u.scan.games.size()),muted);
        if (u.scan.games.empty()) {
            box(r,52,215,1176,380,panel);
            label(r,title,90,270,"Tu biblioteca está vacía",white);
            paragraph(r,normal,90,340,u.scan.error.empty() ?
                "Añade carpetas preparadas a games y pulsa X para reescanear." : u.scan.error,
                muted,70,3);
        } else {
            int shown = u.compact ? 8 : 5;
            int step = u.compact ? 55 : 76;
            size_t start = u.selected >= (size_t)shown ? u.selected - shown + 1 : 0;
            for (int i=0; i<shown && start+(size_t)i<u.scan.games.size(); ++i) {
                size_t index = start+i;
                const Game &g = u.scan.games[index];
                int y=205+i*step;
                box(r,52,y,600,step-7,index==u.selected?accent:panel);
                box(r,56,y+4,592,step-15,panel);
                label(r,normal,74,y+10,g.title,white,470);
                if (!u.compact) label(r,small,74,y+44,g.ready?"Preparado para iniciar":"Revisar instalación",g.ready?accent:warning);
            }
            const Game &g = u.scan.games[u.selected];
            box(r,680,205,548,410,panel);
            game_cover(r,u,g,710,235,185,225);
            label(r,normal,920,248,g.title,white,280);
            label(r,small,920,300,g.author.empty()?"Autor no indicado":g.author,muted,275);
            paragraph(r,small,920,348,g.description,muted,29,5);
            label(r,small,710,482,g.ready?"LISTO PARA ARRANCAR":"INSTALACIÓN INCOMPLETA",g.ready?accent:warning);
            label(r,small,710,545,"A  Detalles     X  Reescanear     +  Ajustes",muted,490);
        }
    } else if (u.view == DETAILS && !u.scan.games.empty()) {
        const Game &g=u.scan.games[u.selected];
        label(r,normal,52,126,"Detalles del juego",muted);
        box(r,52,200,1176,405,panel);
        game_cover(r,u,g,82,230,250,335);
        label(r,title,370,227,g.title,white,815);
        label(r,normal,370,300,g.author.empty()?"Autor no indicado":g.author,muted);
        label(r,small,370,341,g.version.empty()?"Versión no indicada":"Versión " + g.version,muted);
        paragraph(r,normal,370,390,g.description,white,47,3);
        paragraph(r,small,370,510,g.ready?"Estructura preparada. Compatibilidad del juego sin verificar.":g.note,
                  g.ready?accent:warning,73,2);
        label(r,small,370,567,g.ready?"A  Iniciar juego     B  Volver":"B  Volver",muted);
    } else if (u.view == SETTINGS) {
        label(r,normal,52,126,"Ajustes",white);
        box(r,52,200,1176,405,panel);
        box(r,72,225+u.settings*78,1136,64,inset);
        label(r,normal,90,238,std::string("Lista compacta: ")+(u.compact?"Sí":"No"),white);
        label(r,normal,90,316,"Reescanear biblioteca",white);
        label(r,normal,90,394,"Acerca de FangameNX",white);
        label(r,small,90,520,"Motor mkxp-z 2.4.2 + CRuby 3.1.7. FNX-017 pendiente de verificar.",muted);
    } else {
        box(r,52,200,1176,405,panel);
        label(r,title,90,250,"Aviso",warning);
        paragraph(r,normal,90,330,u.message,white,73,5);
        label(r,small,90,548,"B  Volver",muted);
    }
    box(r,50,651,1180,1,inset);
    label(r,small,52,667,"Cruceta  Mover     A  Confirmar     B  Volver",muted);
    label(r,small,1100,667,"M013",accent);
    SDL_RenderPresent(r);
}
static void move(Ui &u, int delta) {
    if (u.view == SETTINGS) u.settings = std::max(0,std::min(2,u.settings+delta));
    else if (u.view == LIBRARY && !u.scan.games.empty()) {
        int next=std::max(0,std::min((int)u.scan.games.size()-1,(int)u.selected+delta));
        u.selected=(size_t)next;
    }
}
static void action(Ui &u, int kind) {
    if (kind == 1) {
        if (u.view == LIBRARY) u.running=false;
        else if (u.view == MESSAGE) u.view=u.message_back;
        else u.view=LIBRARY;
    } else if (kind == 2) {
        if (u.view == LIBRARY) rescan(u);
    } else if (kind == 3) {
        if (u.view == LIBRARY) u.view=SETTINGS;
    } else if (kind == 0) {
        if (u.view == LIBRARY && !u.scan.games.empty()) u.view=DETAILS;
        else if (u.view == DETAILS && !u.scan.games.empty()) {
            const std::string id=u.scan.games[u.selected].id;
            Scan fresh=scan(u.root);
            auto it=std::find_if(fresh.games.begin(),fresh.games.end(),[&](const Game &g){return g.id==id;});
            if (it!=fresh.games.end() && it->ready) {
                u.launch=it->path;
                log_line(u.root,("seleccion: " + id).c_str());
                u.running=false;
            } else {
                u.message="La instalación ha cambiado o está incompleta. Reescanea la biblioteca y revisa los archivos del juego.";
                u.message_back=DETAILS;
                u.view=MESSAGE;
            }
        } else if (u.view == SETTINGS) {
            if (u.settings==0) {
                u.compact=!u.compact;
                if (!save_preference(u.root,u.compact)) {
                    u.message="No se pudo guardar la preferencia en config/launcher.json";
                    u.message_back=SETTINGS;
                    u.view=MESSAGE;
                }
            } else if (u.settings==1) rescan(u);
            else { u.message="FangameNX M013. Juegos externos preparados en games/. El juego vuelve a Homebrew Menu al salir. FNX-017 sigue abierto."; u.message_back=SETTINGS; u.view=MESSAGE; }
        }
    }
}
std::string launcher(const std::string &root) {
    Ui u; u.root=root;
    log_line(root,"inicio: launcher");
    if (!load_preference(root,u.compact)) log_line(root,"config: invalida; valores predeterminados");
    rescan(u);
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER) != 0) return "";
    if (TTF_Init() != 0) { SDL_Quit(); return ""; }
    SDL_Window *window=SDL_CreateWindow("FangameNX",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1280,720,0);
    SDL_Renderer *r=window?SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED):nullptr;
    if (!r && window) r=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    SDL_RWops *fontdata=SDL_RWFromConstMem(___assets_liberation_ttf,___assets_liberation_ttf_len);
    TTF_Font *normal=fontdata?TTF_OpenFontRW(fontdata,1,25):nullptr;
    SDL_RWops *titledata=SDL_RWFromConstMem(___assets_liberation_ttf,___assets_liberation_ttf_len);
    TTF_Font *title=titledata?TTF_OpenFontRW(titledata,1,42):nullptr;
    SDL_RWops *smalldata=SDL_RWFromConstMem(___assets_liberation_ttf,___assets_liberation_ttf_len);
    TTF_Font *small=smalldata?TTF_OpenFontRW(smalldata,1,19):nullptr;
    if (!window || !r || !normal || !title || !small) u.running=false;
    if (r) SDL_RenderSetLogicalSize(r,1280,720);
    SDL_GameController *pad=nullptr;
    for (int i=0;i<SDL_NumJoysticks();++i) if (SDL_IsGameController(i)) { pad=SDL_GameControllerOpen(i); break; }
    Uint32 last=0;
    if (u.running) render(r,title,normal,small,u);
    while (u.running) {
        SDL_Event e;
        if (!SDL_WaitEventTimeout(&e,33)) continue;
        if (e.type==SDL_QUIT) u.running=false;
        if (e.type==SDL_CONTROLLERDEVICEADDED && !pad) pad=SDL_GameControllerOpen(e.cdevice.which);
        if (e.type==SDL_CONTROLLERBUTTONDOWN || e.type==SDL_KEYDOWN) {
            if (e.type==SDL_KEYDOWN && e.key.repeat) continue;
            Uint32 now=SDL_GetTicks();
            if (now-last<140) continue;
            last=now;
            int button=e.type==SDL_CONTROLLERBUTTONDOWN?e.cbutton.button:-1;
            int key=e.type==SDL_KEYDOWN?e.key.keysym.sym:0;
            if (button==SDL_CONTROLLER_BUTTON_DPAD_UP || key==SDLK_UP) move(u,-1);
            else if (button==SDL_CONTROLLER_BUTTON_DPAD_DOWN || key==SDLK_DOWN) move(u,1);
            else if (button==SDL_CONTROLLER_BUTTON_B || key==SDLK_RETURN) action(u,0);
            else if (button==SDL_CONTROLLER_BUTTON_A || key==SDLK_ESCAPE) action(u,1);
            else if (button==SDL_CONTROLLER_BUTTON_Y || key==SDLK_x) action(u,2);
            else if (button==SDL_CONTROLLER_BUTTON_START || key==SDLK_F1) action(u,3);
        }
        if (u.running && e.type != SDL_CONTROLLERDEVICEADDED) render(r,title,normal,small,u);
    }
    if (pad) SDL_GameControllerClose(pad);
    if (u.cover_texture) SDL_DestroyTexture(u.cover_texture);
    if (small) TTF_CloseFont(small);
    if (title) TTF_CloseFont(title);
    if (normal) TTF_CloseFont(normal);
    if (r) SDL_DestroyRenderer(r);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit(); SDL_Quit();
    log_line(root,u.launch.empty()?"salida: hbmenu":"salida: motor seleccionado");
    return u.launch;
}
}
