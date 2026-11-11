/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "launcher_ui.h"
#include "launcher_library.h"
#include <SDL.h>
#include <SDL_touch.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#ifdef __SWITCH__
#include <switch.h>
#endif
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <map>
#include <sys/stat.h>

extern unsigned char ___assets_liberation_ttf[];
extern unsigned int ___assets_liberation_ttf_len;

namespace fnx {
#ifdef __SWITCH__
extern "C" char *fake_heap_start;
extern "C" char *fake_heap_end;
static bool applet_memory() {
    return !envIsNso() &&
        (size_t)(fake_heap_end - fake_heap_start) < 1024ULL * 1024 * 1024;
}
#else
static bool applet_memory() { return false; }
#endif
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
enum View { LIBRARY, DETAILS, SETTINGS, MESSAGE, SCANNING };
struct Ui {
    std::string root, message, last_game;
    Scan scan;
    View view = SCANNING;
    View message_back = LIBRARY;
    View scan_back = LIBRARY;
    size_t selected = 0;
    int sort_mode = 0;
    std::vector<std::string> recent;
    std::map<std::string, bool> favorites;
    int settings = 0;
    bool compact = false;
    bool running = true;
    bool scan_pending = true;
    std::string launch;
    std::map<std::string, SDL_Texture *> cover_textures;
};
static bool cover_dimensions_ok(const std::string &path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0 || info.st_size < 24 || info.st_size > 8 * 1024 * 1024)
        return false;
    FILE *f=fopen(path.c_str(),"rb");
    if (!f) return false;
    unsigned char head[24];
    bool valid=false;
    if (fread(head,1,sizeof(head),f)==sizeof(head)) {
        const unsigned char png[] = {137,80,78,71,13,10,26,10};
        if (memcmp(head,png,8)==0 && memcmp(head+12,"IHDR",4)==0) {
            unsigned w=(unsigned)head[16]<<24 | (unsigned)head[17]<<16 |
                       (unsigned)head[18]<<8 | head[19];
            unsigned h=(unsigned)head[20]<<24 | (unsigned)head[21]<<16 |
                       (unsigned)head[22]<<8 | head[23];
            valid=w>0 && h>0 && w<=2048 && h<=2048;
        } else if (head[0]==0xff && head[1]==0xd8 && fseek(f,2,SEEK_SET)==0) {
            // En JPEG la resolución vive en un segmento SOF, no en la cabecera.
            while (ftell(f)>=0 && ftell(f)<info.st_size) {
                int marker=fgetc(f);
                if (marker!=0xff) break;
                do { marker=fgetc(f); } while (marker==0xff);
                if (marker<0 || marker==0xd9 || marker==0xda) break;
                if (marker==0x01 || (marker>=0xd0 && marker<=0xd7)) continue;
                int hi=fgetc(f), lo=fgetc(f);
                if (hi<0 || lo<0) break;
                int length=(hi<<8)|lo;
                if (length<2 || ftell(f)+length-2>info.st_size) break;
                bool sof=(marker>=0xc0 && marker<=0xc3) ||
                         (marker>=0xc5 && marker<=0xc7) ||
                         (marker>=0xc9 && marker<=0xcb) ||
                         (marker>=0xcd && marker<=0xcf);
                if (sof && length>=7) {
                    int precision=fgetc(f), h1=fgetc(f), h2=fgetc(f), w1=fgetc(f), w2=fgetc(f);
                    if (precision<0 || h1<0 || h2<0 || w1<0 || w2<0) break;
                    int w=(w1<<8)|w2, h=(h1<<8)|h2;
                    valid=precision>0 && w>0 && h>0 && w<=2048 && h<=2048;
                    break;
                }
                if (fseek(f,length-2,SEEK_CUR)!=0) break;
            }
        }
    }
    fclose(f);
    return valid;
}
static void game_cover(SDL_Renderer *r, Ui &u, const Game &g, int x, int y, int w, int h) {
    SDL_Texture *texture = nullptr;
    auto cached = u.cover_textures.find(g.cover);
    if (cached == u.cover_textures.end()) {
        if (!g.cover.empty() && cover_dimensions_ok(g.cover)) {
            SDL_Surface *s=IMG_Load(g.cover.c_str());
            if (s) {
                if (s->w<=2048 && s->h<=2048) texture=SDL_CreateTextureFromSurface(r,s);
                SDL_FreeSurface(s);
            }
        }
        u.cover_textures[g.cover] = texture;
    } else {
        texture = cached->second;
    }
    if (!texture) { cover(r,x,y,w,h,(unsigned)u.selected*17); return; }
    box(r,x,y,w,h,inset);
    int tw=0,th=0;
    SDL_QueryTexture(texture,nullptr,nullptr,&tw,&th);
    if (tw<=0 || th<=0) return;
    float scale=std::min((float)w/tw,(float)h/th);
    SDL_Rect dest={x+(w-(int)(tw*scale))/2,y+(h-(int)(th*scale))/2,
                   (int)(tw*scale),(int)(th*scale)};
    SDL_RenderCopy(r,texture,nullptr,&dest);
}
static void rescan(Ui &u) {
    for (auto &entry : u.cover_textures) if (entry.second) SDL_DestroyTexture(entry.second);
    u.cover_textures.clear();
    const std::string wanted = u.scan.games.empty() ? u.last_game : u.scan.games[u.selected].id;
    u.scan = scan(u.root);
    u.favorites.clear();
    load_recent_games(u.root, u.recent);
    for (const Game &g : u.scan.games) {
        bool favorite = false;
        load_game_favorite(u.root, g.id, favorite);
        u.favorites[g.id] = favorite;
    }
    auto recent_rank = [&](const std::string &id) {
        auto found=std::find(u.recent.begin(),u.recent.end(),id);
        return found==u.recent.end() ? 100 : (int)(found-u.recent.begin());
    };
    std::stable_sort(u.scan.games.begin(),u.scan.games.end(),[&](const Game &a,const Game &b) {
        if (u.sort_mode==1 && u.favorites[a.id]!=u.favorites[b.id])
            return u.favorites[a.id];
        if (u.sort_mode==2 && recent_rank(a.id)!=recent_rank(b.id))
            return recent_rank(a.id)<recent_rank(b.id);
        return a.title<b.title;
    });
    auto it = std::find_if(u.scan.games.begin(), u.scan.games.end(),
        [&](const Game &g) { return g.id == wanted; });
    u.selected = it == u.scan.games.end() ? 0 : (size_t)(it - u.scan.games.begin());
    log_line(u.root, u.scan.error.empty() ? "scan: terminado" : "scan: error de lectura");
    u.scan_pending=false;
    u.view=u.scan_back;
}
static size_t first_visible(const Ui &u) {
    const size_t shown = u.compact ? 9 : 6;
    return u.selected / shown * shown;
}
static void render(SDL_Renderer *r, TTF_Font *title, TTF_Font *normal, TTF_Font *small, Ui &u) {
    const bool limited = applet_memory();
    box(r,0,0,1280,720,bg);
    box(r,0,0,1280,8,accent);
    label(r,title,52,35,"FangameNX",white);
    label(r,small,limited ? 912 : 1000,48,limited ? "MEMORIA LIMITADA" : "LAUNCHER  0.1",limited ? warning : muted,205);
    box(r,1115,25,113,58,inset);
    label(r,small,1144,43,"Salir",white);
    box(r,50,104,1180,1,inset);
    if (u.view == SCANNING) {
        box(r,52,200,1176,405,panel);
        label(r,title,90,270,"Escaneando biblioteca",white);
        label(r,normal,90,350,"Buscando juegos preparados en games/...",muted);
    } else if (u.view == LIBRARY) {
        label(r,normal,52,120,"Biblioteca",white);
        label(r,small,52,158,"Juegos: " + std::to_string(u.scan.games.size()) +
              "    Orden: " + (u.sort_mode==0 ? "A-Z" : u.sort_mode==1 ? "Favoritos" : "Recientes") +
              "    L/R: cambiar",muted);
        if (u.scan.games.empty()) {
            box(r,52,215,1176,380,panel);
            label(r,title,90,270,"Tu biblioteca está vacía",white);
            paragraph(r,normal,90,340,u.scan.error.empty() ?
                "Añade carpetas preparadas a games y pulsa X para reescanear." : u.scan.error,
                muted,70,3);
        } else {
            const int shown = u.compact ? 9 : 6;
            const int height = u.compact ? 136 : 207;
            const int coverWidth = u.compact ? 88 : 125;
            size_t start = first_visible(u);
            for (int i=0; i<shown && start+(size_t)i<u.scan.games.size(); ++i) {
                size_t index = start+i;
                const Game &g = u.scan.games[index];
                const int x=52+(i%3)*398;
                const int y=189+(i/3)*(height+8);
                box(r,x,y,382,height,index==u.selected?accent:inset);
                box(r,x+4,y+4,374,height-8,panel);
                game_cover(r,u,g,x+13,y+12,coverWidth,height-24);
                const int tx=x+coverWidth+27;
                label(r,normal,tx,y+15,g.title,white,346-coverWidth);
                label(r,small,tx,y+64,g.version.empty()?"Versión sin indicar":"Versión " + g.version,
                      muted,346-coverWidth);
                label(r,small,tx,y+105,g.ready?"LISTO PARA JUGAR":"REVISAR INSTALACIÓN",
                      g.ready?accent:warning,346-coverWidth);
                if (u.favorites[g.id]) label(r,small,tx,y+height-34,"FAVORITO",accent,346-coverWidth);
            }
            label(r,small,870,158,"Página " + std::to_string(start/shown+1) + " / " +
                  std::to_string((u.scan.games.size()+shown-1)/shown),muted,340);
        }
    } else if (u.view == DETAILS && !u.scan.games.empty()) {
        const Game &g=u.scan.games[u.selected];
        label(r,normal,52,126,"Detalles del juego",muted);
        box(r,52,200,1176,405,panel);
        game_cover(r,u,g,82,230,250,335);
        label(r,title,370,227,g.title,white,815);
        label(r,normal,370,300,g.author.empty()?"Autor no indicado":g.author,muted);
        label(r,small,370,341,g.version.empty()?"Versión no indicada":"Versión " + g.version,muted);
        label(r,small,760,341,"Motor: mkxp-z / RGSS",muted);
        paragraph(r,normal,370,390,g.description,white,47,3);
        paragraph(r,small,370,510,g.ready?"Estructura preparada. Compatibilidad del juego sin verificar.":g.note,
                  g.ready?accent:warning,73,2);
        if (g.ready) {
            box(r,370,548,205,58,inset);
            label(r,small,390,565,limited?"Sin memoria":"Iniciar juego",white);
        }
        box(r,590,548,170,58,inset);
        label(r,small,635,565,"Volver",white);
        box(r,780,548,206,58,inset);
        label(r,small,794,565,u.favorites[g.id]?"Quitar favorito":"Añadir favorito",white);
    } else if (u.view == SETTINGS) {
        label(r,normal,52,126,"Ajustes",white);
        box(r,52,200,1176,405,panel);
        box(r,72,225+u.settings*78,1136,64,inset);
        label(r,normal,90,238,std::string("Lista compacta: ")+(u.compact?"Sí":"No"),white);
        label(r,normal,90,316,"Reescanear biblioteca",white);
        label(r,normal,90,394,"Acerca de FangameNX",white);
        box(r,90,515,170,58,inset);
        label(r,small,135,532,"Volver",white);
        label(r,small,290,532,"Motor mkxp-z 2.4.2 + CRuby 3.1.7",muted);
    } else {
        box(r,52,200,1176,405,panel);
        label(r,title,90,250,"Aviso",warning);
        paragraph(r,normal,90,330,u.message,white,73,5);
        box(r,90,530,170,58,inset);
        label(r,small,135,547,"Volver",white);
    }
    box(r,50,651,1180,1,inset);
    label(r,small,52,667,"Cruceta  Mover     B  Elegir     A  Volver     X  Reescanear     Y  Ajustes",muted);
    label(r,small,1100,667,"M023",accent);
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
        if (u.view == MESSAGE) u.view=u.message_back;
        else if (u.view == LIBRARY) return;
        else u.view=LIBRARY;
    } else if (kind == 2) {
        if (u.view == LIBRARY) { u.scan_back=LIBRARY; u.view=SCANNING; u.scan_pending=true; }
    } else if (kind == 3) {
        if (u.view == LIBRARY) u.view=SETTINGS;
    } else if (kind == 4) {
        u.running=false;
    } else if (kind == 5) {
        if (u.view == DETAILS && !u.scan.games.empty()) {
            const std::string id=u.scan.games[u.selected].id;
            if (save_game_favorite(u.root,id,!u.favorites[id])) {
                u.scan_back=DETAILS; u.view=SCANNING; u.scan_pending=true;
            } else {
                u.message="No se pudo guardar el favorito.";
                u.message_back=DETAILS; u.view=MESSAGE;
            }
        }
    } else if (kind == 6) {
        if (u.view == LIBRARY) {
            u.sort_mode=(u.sort_mode+1)%3;
            u.scan_back=LIBRARY; u.view=SCANNING; u.scan_pending=true;
        }
    } else if (kind == 0) {
        if (u.view == LIBRARY && !u.scan.games.empty()) u.view=DETAILS;
        else if (u.view == DETAILS && !u.scan.games.empty()) {
            const std::string id=u.scan.games[u.selected].id;
            Scan fresh=scan(u.root);
            auto it=std::find_if(fresh.games.begin(),fresh.games.end(),[&](const Game &g){return g.id==id;});
            if (it!=fresh.games.end() && it->ready) {
                if (applet_memory()) {
                    u.message="Memoria insuficiente: FangameNX se abrió como applet. Abre Homebrew Menu manteniendo R al iniciar un juego instalado y vuelve a abrir FangameNX. El motor necesita memoria de aplicación.";
                    u.message_back=DETAILS;
                    u.view=MESSAGE;
                    log_line(u.root,"arranque bloqueado: memoria de applet");
                    return;
                }
                if (!save_last_game(u.root,id)) {
                    char issue[100];
                    snprintf(issue,sizeof(issue),"config: no se pudo recordar el ultimo juego; errno=%d",errno);
                    log_line(u.root,issue);
                }
                if (!record_recent_game(u.root,id)) log_line(u.root,"config: no se pudo registrar juego reciente");
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
            } else if (u.settings==1) { u.scan_back=SETTINGS; u.view=SCANNING; u.scan_pending=true; }
            else { u.message="FangameNX. Juegos preparados en games/. Durante un juego, mantén L+R+PLUS 0,8 s para abrir el menú rápido y elegir Volver a la biblioteca. B sigue disponible para el juego. La incidencia FNX-017 sigue abierta."; u.message_back=SETTINGS; u.view=MESSAGE; }
        }
    }
}
static bool hit(float x, float y, int left, int top, int width, int height) {
    return x >= left && x < left + width && y >= top && y < top + height;
}
static void touch(Ui &u, float x, float y) {
    if (hit(x,y,1115,25,113,58)) { action(u,4); return; }
    if (u.view == LIBRARY && !u.scan.games.empty()) {
        const int shown = u.compact ? 9 : 6;
        const int height = u.compact ? 136 : 207;
        const size_t start = first_visible(u);
        for (int i=0; i<shown && start+(size_t)i<u.scan.games.size(); ++i) {
            if (hit(x,y,52+(i%3)*398,189+(i/3)*(height+8),382,height)) {
                const size_t index = start+(size_t)i;
                if (u.selected == index) action(u,0);
                else u.selected = index;
                return;
            }
        }
    } else if (u.view == DETAILS) {
        if (hit(x,y,590,548,170,58)) { action(u,1); return; }
        if (hit(x,y,780,548,206,58)) { action(u,5); return; }
        if (!u.scan.games.empty() && u.scan.games[u.selected].ready &&
            hit(x,y,370,548,205,58)) { action(u,0); return; }
    } else if (u.view == SETTINGS) {
        if (hit(x,y,90,515,170,58)) { action(u,1); return; }
        for (int i=0; i<3; ++i) {
            if (hit(x,y,72,225+i*78,1136,64)) {
                u.settings=i;
                action(u,0);
                return;
            }
        }
    } else if (u.view == MESSAGE && hit(x,y,90,530,170,58)) {
        action(u,1);
    }
}
std::string launcher(const std::string &root) {
    Ui u; u.root=root;
    log_line(root,"inicio: launcher");
    if (!load_preference(root,u.compact)) log_line(root,"config: invalida; valores predeterminados");
    if (!load_last_game(root,u.last_game)) log_line(root,"config: ultimo juego invalido; foco inicial predeterminado");
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
        if (u.scan_pending) { rescan(u); render(r,title,normal,small,u); }
        SDL_Event e;
        if (!SDL_WaitEventTimeout(&e,33)) continue;
        if (e.type==SDL_QUIT) u.running=false;
        if (e.type==SDL_CONTROLLERDEVICEADDED && !pad) pad=SDL_GameControllerOpen(e.cdevice.which);
        if (e.type==SDL_FINGERUP ||
            (e.type==SDL_MOUSEBUTTONUP && e.button.which!=SDL_TOUCH_MOUSEID)) {
            Uint32 now=SDL_GetTicks();
            if (now-last>=140) {
                last=now;
                float x=0,y=0;
                if (e.type==SDL_FINGERUP) {
                    int w=0,h=0;
                    SDL_GetWindowSize(window,&w,&h);
                    SDL_RenderWindowToLogical(r,e.tfinger.x*w,e.tfinger.y*h,&x,&y);
                } else {
                    SDL_RenderWindowToLogical(r,(float)e.button.x,(float)e.button.y,&x,&y);
                }
                touch(u,x,y);
            }
        }
        if (e.type==SDL_CONTROLLERBUTTONDOWN || e.type==SDL_KEYDOWN) {
            if (e.type==SDL_KEYDOWN && e.key.repeat) continue;
            Uint32 now=SDL_GetTicks();
            if (now-last<140) continue;
            last=now;
            int button=e.type==SDL_CONTROLLERBUTTONDOWN?e.cbutton.button:-1;
            int key=e.type==SDL_KEYDOWN?e.key.keysym.sym:0;
            if (button==SDL_CONTROLLER_BUTTON_DPAD_UP || key==SDLK_UP) move(u,u.view==LIBRARY?-3:-1);
            else if (button==SDL_CONTROLLER_BUTTON_DPAD_DOWN || key==SDLK_DOWN) move(u,u.view==LIBRARY?3:1);
            else if (button==SDL_CONTROLLER_BUTTON_DPAD_LEFT || key==SDLK_LEFT) move(u,-1);
            else if (button==SDL_CONTROLLER_BUTTON_DPAD_RIGHT || key==SDLK_RIGHT) move(u,1);
            else if (button==SDL_CONTROLLER_BUTTON_B || key==SDLK_RETURN) action(u,0);
            else if (button==SDL_CONTROLLER_BUTTON_A || key==SDLK_ESCAPE) action(u,1);
            else if (button==SDL_CONTROLLER_BUTTON_Y || key==SDLK_x) action(u,2);
            else if (button==SDL_CONTROLLER_BUTTON_X || key==SDLK_F1)
                action(u,u.view==DETAILS?5:3);
            else if (button==SDL_CONTROLLER_BUTTON_LEFTSHOULDER ||
                     button==SDL_CONTROLLER_BUTTON_RIGHTSHOULDER || key==SDLK_TAB) action(u,6);
            else if (button==SDL_CONTROLLER_BUTTON_START || key==SDLK_F10) action(u,4);
        }
        if (u.running && e.type != SDL_CONTROLLERDEVICEADDED) render(r,title,normal,small,u);
    }
    if (pad) SDL_GameControllerClose(pad);
    for (auto &entry : u.cover_textures) if (entry.second) SDL_DestroyTexture(entry.second);
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
