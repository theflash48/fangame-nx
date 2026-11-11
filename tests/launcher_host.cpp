/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "launcher_library.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs=std::filesystem;
static void put(const fs::path &p, const std::string &text) {
    fs::create_directories(p.parent_path());
    std::ofstream f(p,std::ios::binary);
    assert(f);
    f<<text;
}
int main(int argc,char **argv) {
    assert(argc==3);
    fs::path root=fs::path(argv[1])/std::to_string(std::time(nullptr));
    fs::create_directories(root/"games");
    auto empty=fnx::scan(root.string());
    assert(empty.error.empty() && empty.games.empty());
    assert(!fnx::safe_child("../escape") && !fnx::safe_child("a/b") &&
           !fnx::safe_child("."));
    fs::path alpha=root/"games"/"alpha";
    put(alpha/"Game.ini","[Game]\nTitle=Alfa sintético\n");
    put(alpha/"mkxp.json","{}");
    put(alpha/"lib"/"nx-compat.rb","");
    put(alpha/"lib"/"socket.rb","");
    put(alpha/"Data"/"Scripts.rxdata","");
    fs::create_directories(alpha/"Graphics");
    fs::create_directories(alpha/"Audio");
    fs::path broken=root/"games"/"broken";
    put(broken/"fangamenx.json","{invalid json");
    auto all=fnx::scan(root.string());
    assert(all.games.size()==2 && all.games[0].id=="alpha" &&
           all.games[0].ready && all.games[1].id=="broken" && !all.games[1].ready);
    put(alpha/"cover.png","imagen de prueba");
    all=fnx::scan(root.string());
    assert(fs::equivalent(all.games[0].cover,alpha/"cover.png"));
    put(alpha/"fangamenx.json","{\"id\":\"alpha\",\"name\":\"Título alternativo\",\"author\":\"Equipo\"}");
    all=fnx::scan(root.string());
    assert(all.games[0].title=="Título alternativo" && all.games[0].ready &&
           fs::equivalent(all.games[0].cover,alpha/"cover.png"));
    put(alpha/"other.png","imagen explícita de prueba");
    put(alpha/"fangamenx.json","{\"cover\":\"other.png\"}");
    all=fnx::scan(root.string());
    assert(fs::equivalent(all.games[0].cover,alpha/"other.png"));
    put(alpha/"fangamenx.json",std::string("{\"name\":\"")+std::string(95,'N')+"\"}");
    all=fnx::scan(root.string());
    assert(all.games[0].title.size()==95 && all.games[0].ready);
    put(alpha/"fangamenx.json","{\"cover\":\"../escape.png\"}");
    all=fnx::scan(root.string());
    assert(!all.games[0].ready && all.games[0].cover.empty());
    put(alpha/"fangamenx.json","{\"id\":\"../escape\"}");
    all=fnx::scan(root.string());
    assert(!all.games[0].ready);
    bool compact=false;
    std::string last;
    assert(fnx::load_preference(root.string(),compact) && !compact);
    assert(fnx::load_last_game(root.string(),last) && last.empty());
    assert(fnx::save_preference(root.string(),true));
    assert(fnx::load_preference(root.string(),compact) && compact);
    assert(fnx::save_last_game(root.string(),"alpha"));
    assert(fnx::load_last_game(root.string(),last) && last=="alpha");
    assert(!fnx::save_last_game(root.string(),"../escape"));
    put(root/"config"/"launcher.json","{\"schema\":1,\"compact\":true,\"future\":42}");
    assert(fnx::save_last_game(root.string(),"broken"));
    assert(fnx::save_preference(root.string(),false));
    std::ifstream in(root/"config"/"launcher.json");
    std::string data((std::istreambuf_iterator<char>(in)),{});
    assert(data.find("future")!=std::string::npos);
    in.close();
    assert(fnx::load_last_game(root.string(),last) && last=="broken");
    fs::rename(root/"config"/"launcher.json",root/"config"/"launcher.json.bak");
    assert(fnx::load_last_game(root.string(),last) && last=="broken");
    assert(fnx::save_last_game(root.string(),"alpha"));
    assert(fnx::load_last_game(root.string(),last) && last=="alpha");
    assert(!fs::exists(root/"config"/"launcher.json.bak"));
    bool favorite=true;
    assert(fnx::load_game_favorite(root.string(),"alpha",favorite) && !favorite);
    assert(fnx::save_game_favorite(root.string(),"alpha",true));
    assert(fnx::load_game_favorite(root.string(),"alpha",favorite) && favorite);
    assert(fnx::record_recent_game(root.string(),"alpha"));
    assert(fnx::record_recent_game(root.string(),"broken"));
    assert(fnx::record_recent_game(root.string(),"alpha"));
    std::vector<std::string> recent;
    assert(fnx::load_recent_games(root.string(),recent));
    assert(recent.size()==2 && recent[0]=="alpha" && recent[1]=="broken");
    assert(!fnx::record_recent_game(root.string(),"../escape"));
    put(root/"config"/"launcher.json","{\"schema\":1,\"compact\":false,\"last_game\":\"../escape\"}");
    assert(!fnx::load_last_game(root.string(),last) && last.empty());
    auto packaged=fnx::scan(argv[2]);
    assert(packaged.error.empty() && packaged.games.size()==2);
    assert(packaged.games[0].ready && packaged.games[1].ready);
    std::cout<<"launcher_host: OK\n";
}
