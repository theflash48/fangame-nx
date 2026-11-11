/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FNX_LAUNCHER_LIBRARY_H
#define FNX_LAUNCHER_LIBRARY_H

#include <string>
#include <vector>

namespace fnx {
struct Game {
    std::string id, title, description, author, version, path, cover, note;
    bool ready = false;
};

struct Scan {
    std::vector<Game> games;
    std::string error;
};

/* root es la carpeta de la instalación, nunca una ruta aportada por un juego. */
Scan scan(const std::string &root);
bool safe_child(const std::string &name);
bool load_preference(const std::string &root, bool &compact);
bool save_preference(const std::string &root, bool compact);
bool load_last_game(const std::string &root, std::string &id);
bool save_last_game(const std::string &root, const std::string &id);
/* 0=automático, 1=0,5x, 2=1x, 3=1,5x, 4=2x. Preferencia por ID de juego. */
bool load_game_speed(const std::string &root, const std::string &id, int &mode);
bool save_game_speed(const std::string &root, const std::string &id, int mode);
/* Cuatro letras ABXY: indice = boton logico, valor = boton fisico. */
bool load_game_controls(const std::string &root, const std::string &id, std::string &mapping);
bool save_game_controls(const std::string &root, const std::string &id, const std::string &mapping);
/* Perfil SDL: 15 botones lógicos, fuente física A..Q (P/Q = ZL/ZR). GUIDE (5)
   permanece reservado. Migra controls_<id> de b36 al cargar. */
bool load_game_buttonmap(const std::string &root, const std::string &id, std::string &mapping);
bool save_game_buttonmap(const std::string &root, const std::string &id, const std::string &mapping);
/* Opciones de sticks por juego: swap, invert X/Y por stick y zona muerta. */
bool load_game_sticks(const std::string &root, const std::string &id, int &flags, int &deadzone);
bool save_game_sticks(const std::string &root, const std::string &id, int flags, int deadzone);
bool load_game_favorite(const std::string &root, const std::string &id, bool &favorite);
bool save_game_favorite(const std::string &root, const std::string &id, bool favorite);
bool load_recent_games(const std::string &root, std::vector<std::string> &ids);
bool record_recent_game(const std::string &root, const std::string &id);
bool load_sort_mode(const std::string &root, int &mode);
bool save_sort_mode(const std::string &root, int mode);
}
#endif
