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
}
#endif
