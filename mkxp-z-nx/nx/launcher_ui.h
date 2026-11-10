/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FNX_LAUNCHER_UI_H
#define FNX_LAUNCHER_UI_H
#include <string>
namespace fnx {
/* Devuelve una carpeta validada o vacío para salir a hbmenu. */
std::string launcher(const std::string &root);
}
#endif
