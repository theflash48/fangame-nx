/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "launcher_library.h"
#include "util/json5pp.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace fnx {
static bool regular(const std::string &p) {
#ifdef _WIN32
    DWORD a = GetFileAttributesA(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT));
#else
    struct stat s;
    return lstat(p.c_str(), &s) == 0 && S_ISREG(s.st_mode);
#endif
}
static bool directory(const std::string &p) {
#ifdef _WIN32
    DWORD a = GetFileAttributesA(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) && !(a & FILE_ATTRIBUTE_REPARSE_POINT);
#else
    struct stat s;
    return lstat(p.c_str(), &s) == 0 && S_ISDIR(s.st_mode);
#endif
}
bool safe_child(const std::string &n) {
    if (n.empty() || n.size() > 80 || n == "." || n == "..") return false;
    for (unsigned char c : n)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_')) return false;
    return true;
}
static std::string read_limited(const std::string &p, size_t max) {
    std::ifstream in(p, std::ios::binary);
    if (!in) throw std::runtime_error("No se puede leer el archivo");
    std::string data;
    char c;
    while (in.get(c)) {
        if (data.size() == max) throw std::runtime_error("Archivo demasiado grande");
        data += c;
    }
    if (in.bad()) throw std::runtime_error("Error de lectura");
    return data;
}
static std::string field(const json5pp::value &v, const char *key, size_t max) {
    const auto &o = v.as_object();
    auto it = o.find(key);
    if (it == o.end()) return "";
    if (!it->second.is_string()) throw std::runtime_error(std::string("Campo no textual: ") + key);
    std::string s = it->second.as_string();
    if (s.size() > max) throw std::runtime_error(std::string("Campo demasiado largo: ") + key);
    return s;
}
static std::string ini_title(const std::string &p) {
    if (!regular(p)) return "";
    std::istringstream in(read_limited(p, 16384));
    std::string line;
    while (std::getline(in, line)) {
        if (line.compare(0, 6, "Title=") == 0) {
            std::string title = line.substr(6);
            if (!title.empty() && title.back() == '\r') title.pop_back();
            if (title.size() > 96) title.resize(96);
            return title;
        }
    }
    return "";
}
static bool safe_cover(const std::string &name) {
    if (name.empty() || name.size()>80 || name[0]=='.' || name.find("..")!=std::string::npos)
        return false;
    for (unsigned char c:name)
        if (!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||
              c=='-'||c=='_'||c=='.')) return false;
    size_t dot=name.rfind('.');
    if (dot==std::string::npos) return false;
    std::string ext=name.substr(dot);
    return ext==".png" || ext==".jpg" || ext==".jpeg";
}
static Game inspect(const std::string &root, const std::string &id) {
    Game g;
    g.id = id;
    g.title = id;
    g.path = root + "/games/" + id;
    const std::string manifest = g.path + "/fangamenx.json";
    try {
        if (regular(manifest)) {
            json5pp::value m = json5pp::parse(read_limited(manifest, 32768));
            if (!m.is_object()) throw std::runtime_error("El manifiesto debe ser un objeto");
            std::string declared = field(m, "id", 80);
            if (!declared.empty() && declared != id)
                throw std::runtime_error("El id debe coincidir con la carpeta");
            std::string t = field(m, "name", 96);
            if (!t.empty()) g.title = t;
            g.description = field(m, "description", 512);
            g.author = field(m, "author", 96);
            g.version = field(m, "version", 40);
            std::string cover = field(m, "cover", 80);
            if (!cover.empty()) {
                if (!safe_cover(cover)) throw std::runtime_error("Ruta de portada no válida");
                if (regular(g.path+"/"+cover)) g.cover=g.path+"/"+cover;
            }
        } else {
            std::string t = ini_title(g.path + "/Game.ini");
            if (!t.empty()) g.title = t;
        }
    } catch (const std::exception &e) {
        g.note = std::string("Metadatos no válidos: ") + e.what();
    }
    if (!regular(g.path + "/mkxp.json")) g.note += " Falta mkxp.json.";
    if (!directory(g.path + "/lib") || !regular(g.path + "/lib/nx-compat.rb"))
        g.note += " Falta lib/nx-compat.rb.";
    if (!regular(g.path + "/lib/socket.rb")) g.note += " Falta lib/socket.rb.";
    bool loose = regular(g.path + "/Data/Scripts.rxdata") &&
                 directory(g.path + "/Graphics") && directory(g.path + "/Audio");
    bool packed = regular(g.path + "/datos.zip") && regular(g.path + "/Game.rgssad");
    if (!loose && !packed) g.note += " Faltan datos preparados.";
    g.ready = g.note.empty();
    if (g.description.empty()) g.description = "Instalación externa de RPG Maker XP";
    return g;
}
Scan scan(const std::string &root) {
    Scan result;
    const std::string path = root + "/games";
    if (!directory(path)) {
        result.error = "No existe una carpeta games válida";
        return result;
    }
#ifdef _WIN32
    WIN32_FIND_DATAA entry;
    HANDLE found=FindFirstFileA((path+"/*").c_str(),&entry);
    if (found == INVALID_HANDLE_VALUE) { result.error="No se puede leer games"; return result; }
    do {
        std::string id=entry.cFileName;
        if (safe_child(id) && directory(path+"/"+id)) result.games.push_back(inspect(root,id));
    } while (FindNextFileA(found,&entry));
    FindClose(found);
#else
    DIR *dir = opendir(path.c_str());
    if (!dir) { result.error = "No se puede leer games"; return result; }
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        std::string id = entry->d_name;
        if (safe_child(id) && directory(path + "/" + id)) result.games.push_back(inspect(root, id));
    }
    closedir(dir);
#endif
    std::sort(result.games.begin(), result.games.end(), [](const Game &a, const Game &b) {
        return a.id < b.id;
    });
    return result;
}
static std::string config_read_path(const std::string &root) {
    std::string path = root + "/config/launcher.json";
    if (regular(path)) return path;
    std::string backup = path + ".bak";
    return regular(backup) ? backup : path;
}
bool load_preference(const std::string &root, bool &compact) {
    std::string path = config_read_path(root);
    if (!regular(path)) return true;
    try {
        auto v = json5pp::parse(read_limited(path, 4096));
        if (!v.is_object()) return false;
        const auto &o = v.as_object();
        auto schema = o.find("schema");
        auto mode = o.find("compact");
        if (schema == o.end() || !schema->second.is_number() ||
            schema->second.as_integer() != 1 || mode == o.end() ||
            !mode->second.is_boolean()) return false;
        compact = mode->second.as_boolean();
        return true;
    } catch (...) { return false; }
}
bool load_last_game(const std::string &root, std::string &id) {
    id.clear();
    std::string path = config_read_path(root);
    if (!regular(path)) return true;
    try {
        auto v = json5pp::parse(read_limited(path, 4096));
        if (!v.is_object()) return false;
        const auto &o = v.as_object();
        auto schema = o.find("schema");
        if (schema == o.end() || !schema->second.is_number() ||
            schema->second.as_integer() != 1) return false;
        auto last = o.find("last_game");
        if (last == o.end()) return true;
        if (!last->second.is_string() || !safe_child(last->second.as_string())) return false;
        id = last->second.as_string();
        return true;
    } catch (...) { return false; }
}
static bool save_config_value(const std::string &root, const char *key, const json5pp::value &value) {
    std::string folder = root + "/config";
#ifdef _WIN32
    if (_mkdir(folder.c_str()) != 0 && errno != EEXIST) return false;
#else
    if (mkdir(folder.c_str(), 0755) != 0 && errno != EEXIST) return false;
#endif
    std::string tmp = folder + "/launcher.json.tmp";
    std::string dst = folder + "/launcher.json";
    std::string backup = dst + ".bak";
    json5pp::value v;
    try {
        const std::string source = config_read_path(root);
        v = regular(source) ? json5pp::parse(read_limited(source, 4096)) :
            json5pp::parse("{\"schema\":1,\"compact\":false}");
        if (!v.is_object()) return false;
        auto &o = v.as_object();
        auto schema = o.find("schema");
        if (schema == o.end() || !schema->second.is_number() ||
            schema->second.as_integer() != 1) return false;
        o[key] = value;
    } catch (...) { return false; }
    FILE *f = fopen(tmp.c_str(), "wb");
    if (!f) return false;
    std::string body = json5pp::stringify(v) + "\n";
    bool ok = fwrite(body.data(), 1, body.size(), f) == body.size();
    ok = (fflush(f) == 0) && ok;
#ifdef _WIN32
    if (ok) ok = _commit(_fileno(f)) == 0;
#else
    if (ok) ok = fsync(fileno(f)) == 0;
#endif
    ok = (fclose(f) == 0) && ok;
    /* Horizon no sustituye un destino existente con rename. Se conserva el
       JSON anterior como respaldo hasta que el temporal pasa a ser el activo.
       Tras una interrupcion entre ambos renombres, las lecturas usan .bak. */
    bool moved_old = false;
    if (ok && regular(dst)) {
        if (regular(backup) && remove(backup.c_str()) != 0) ok = false;
        if (ok) {
            moved_old = rename(dst.c_str(), backup.c_str()) == 0;
            ok = moved_old;
        }
    }
    if (ok) ok = rename(tmp.c_str(), dst.c_str()) == 0;
    const int error_code = ok ? 0 : errno;
    if (!ok && moved_old) rename(backup.c_str(), dst.c_str());
    if (ok && regular(backup)) remove(backup.c_str());
    if (!ok) remove(tmp.c_str());
    if (!ok) errno = error_code;
    return ok;
}
bool save_preference(const std::string &root, bool compact) {
    return save_config_value(root, "compact", compact);
}
bool save_last_game(const std::string &root, const std::string &id) {
    return safe_child(id) && save_config_value(root, "last_game", id);
}
}
