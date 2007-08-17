/*
 *  The Mana World Server
 *  Copyright 2007 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana World is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana World; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id$
 */

#include <cassert>

extern "C" {
#include <lualib.h>
#include <lauxlib.h>
}

#include "defines.h"
#include "game-server/buysell.hpp"
#include "game-server/character.hpp"
#include "game-server/gamehandler.hpp"
#include "game-server/inventory.hpp"
#include "game-server/item.hpp"
#include "game-server/itemmanager.hpp"
#include "game-server/mapmanager.hpp"
#include "game-server/npc.hpp"
#include "game-server/state.hpp"
#include "net/messageout.hpp"
#include "scripting/script.hpp"
#include "utils/logger.h"

/**
 * Implementation of the Script class for Lua.
 */
class LuaScript: public Script
{
    public:

        LuaScript();

        ~LuaScript();

        void load(char const *);

        void prepare(std::string const &);

        void push(int);

        void push(Thing *);

        int execute();

    private:

        lua_State *mState;
        int nbArgs;
};

static char const registryKey = 0;

/* Functions below are unsafe, as they assume the script has passed pointers
   to objects which have not yet been destroyed. If the script never keeps
   pointers around, there will be no problem. In order to be safe, the engine
   should replace pointers by local identifiers and store them in a map. By
   listening to the death of objects, it could keep track of pointers still
   valid in the map.
   TODO: do it. */

static NPC *getNPC(lua_State *s, int p)
{
    if (!lua_islightuserdata(s, p)) return NULL;
    Thing *t = static_cast<Thing *>(lua_touserdata(s, p));
    if (t->getType() != OBJECT_NPC) return NULL;
    return static_cast<NPC *>(t);
}

static Character *getCharacter(lua_State *s, int p)
{
    if (!lua_islightuserdata(s, p)) return NULL;
    Thing *t = static_cast<Thing *>(lua_touserdata(s, p));
    if (t->getType() != OBJECT_CHARACTER) return NULL;
    return static_cast<Character *>(t);
}

/**
 * Callback for sending a NPC_MESSAGE.
 * tmw.msg_npc_message(npc, character, string)
 */
static int LuaMsg_NpcMessage(lua_State *s)
{
    NPC *p = getNPC(s, 1);
    Character *q = getCharacter(s, 2);
    size_t l;
    char const *m = lua_tolstring(s, 3, &l);
    if (!p || !q || !m)
    {
        LOG_WARN("LuaMsg_NpcMessage called with incorrect parameters.");
        return 0;
    }
    MessageOut msg(GPMSG_NPC_MESSAGE);
    msg.writeShort(p->getPublicID());
    msg.writeString(std::string(m), l);
    gameHandler->sendTo(q, msg);
    return 0;
}

/**
 * Callback for sending a NPC_CHOICE.
 * tmw.msg_npc_choice(npc, character, string...)
 */
static int LuaMsg_NpcChoice(lua_State *s)
{
    NPC *p = getNPC(s, 1);
    Character *q = getCharacter(s, 2);
    if (!p || !q)
    {
        LOG_WARN("LuaMsg_NpcChoice called with incorrect parameters.");
        return 0;
    }
    MessageOut msg(GPMSG_NPC_CHOICE);
    msg.writeShort(p->getPublicID());
    for (int i = 3, i_end = lua_gettop(s); i <= i_end; ++i)
    {
        char const *m = lua_tostring(s, i);
        if (!m)
        {
            LOG_WARN("LuaMsg_NpcChoice called with incorrect parameters.");
            return 0;
        }
        msg.writeString(m);
    }
    gameHandler->sendTo(q, msg);
    return 0;
}

/**
 * Callback for creating a NPC on the current map with the current script.
 * tmw.obj_create_npc(int id, int x, int y): npc
 */
static int LuaObj_CreateNpc(lua_State *s)
{
    if (!lua_isnumber(s, 1) || !lua_isnumber(s, 2) || !lua_isnumber(s, 3))
    {
        LOG_WARN("LuaObj_CreateNpc called with incorrect parameters.");
        return 0;
    }
    lua_pushlightuserdata(s, (void *)&registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *t = static_cast<Script *>(lua_touserdata(s, -1));
    NPC *q = new NPC(lua_tointeger(s, 1), t);
    MapComposite *m = t->getMap();
    if (!m)
    {
        LOG_WARN("LuaObj_CreateNpc called outside a map.");
        return 0;
    }
    q->setMap(m);
    q->setPosition(Point(lua_tointeger(s, 2), lua_tointeger(s, 3)));
    GameState::insert(q);
    lua_pushlightuserdata(s, q);
    return 1;
}

/**
 * Callback for warping a player to another place.
 * tmw.chr_warp(character, nil/int map, int x, int y)
 */
static int LuaChr_Warp(lua_State *s)
{
    Character *q = getCharacter(s, 1);
    bool b = lua_isnil(s, 2);
    if (!q || !(b || lua_isnumber(s, 2)) ||
        !lua_isnumber(s, 3) || !lua_isnumber(s, 4))
    {
        LOG_WARN("LuaChr_Warp called with incorrect parameters.");
        return 0;
    }
    MapComposite *m;
    if (b)
    {
        lua_pushlightuserdata(s, (void *)&registryKey);
        lua_gettable(s, LUA_REGISTRYINDEX);
        Script *t = static_cast<Script *>(lua_touserdata(s, -1));
        m = t->getMap();
    }
    else
    {
        m = MapManager::getMap(lua_tointeger(s, 2));
    }
    if (!m)
    {
        LOG_WARN("LuaChr_Warp called with a non-existing map.");
        return 0;
    }
    DelayedEvent e = { EVENT_WARP, lua_tointeger(s, 3), lua_tointeger(s, 4), m };
    GameState::enqueueEvent(q, e);
    return 0;
}

/**
 * Callback for inserting/removing items in inventory.
 * The function can be called several times in a row, but it is better to
 * perform all the changes at once, so as to reduce bandwidth. Removals
 * (negative amount) should be passed first, then insertions (positive amount).
 * If a removal fails, all the previous operations are canceled (except for
 * items dropped on the floor, hence why removals should be passed first), and
 * the function returns false. Otherwise the function will return true.
 * Note: If an insertion fails, extra items are dropped on the floor.
 * tmw.chr_inv_change(character, (int id, int nb)...): bool success
 */
static int LuaChr_InvChange(lua_State *s)
{
    Character *q = getCharacter(s, 1);
    if (!q)
    {
        LOG_WARN("LuaChr_InvChange called with incorrect parameters.");
        return 0;
    }
    int nb_items = (lua_gettop(s) - 1) / 2;
    Inventory inv(q, true);
    for (int i = 0; i < nb_items; ++i)
    {
        if (!lua_isnumber(s, i * 2 + 2) || !lua_isnumber(s, i * 2 + 3))
        {
            LOG_WARN("LuaChr_InvChange called with incorrect parameters.");
            return 0;
        }
        int id = lua_tointeger(s, i * 2 + 2);
        int nb = lua_tointeger(s, i * 2 + 3);
        if (nb < 0)
        {
            nb = inv.remove(id, -nb);
            if (nb)
            {
                inv.cancel();
                lua_pushboolean(s, 0);
                return 1;
            }
        }
        else
        {
            ItemClass *ic = ItemManager::getItem(id);
            if (!ic)
            {
                LOG_WARN("LuaChr_InvChange called with an unknown item.");
                continue;
            }
            nb = inv.insert(id, nb);
            if (nb)
            {
                Item *item = new Item(ic, nb);
                item->setMap(q->getMap());
                item->setPosition(q->getPosition());
                DelayedEvent e = { EVENT_INSERT };
                GameState::enqueueEvent(item, e);
            }
        }
    }
    lua_pushboolean(s, 1);
    return 1;
}

/**
 * Callback for counting items in inventory.
 * tmw.chr_inv_count(character, int id...): int count...
 */
static int LuaChr_InvCount(lua_State *s)
{
    Character *q = getCharacter(s, 1);
    if (!q)
    {
        LOG_WARN("LuaChr_InvCount called with incorrect parameters.");
        return 0;
    }
    int nb_items = lua_gettop(s) - 1;
    lua_checkstack(s, nb_items);
    Inventory inv(q);
    for (int i = 2; i <= nb_items + 1; ++i)
    {
        if (!lua_isnumber(s, i))
        {
            LOG_WARN("LuaChr_InvCount called with incorrect parameters.");
            return 0;
        }
        int nb = inv.count(lua_tointeger(s, i));
        lua_pushinteger(s, nb);
    }
    return nb_items;
}

/**
 * Callback for trading between a a player and an NPC.
 * tmw.npc_trade(npc, character, bool sell, table items)
 */
static int LuaNpc_Trade(lua_State *s)
{
    NPC *p = getNPC(s, 1);
    Character *q = getCharacter(s, 2);
    if (!p || !q || !lua_isboolean(s, 3) || !lua_istable(s, 4))
    {
        LOG_WARN("LuaNpc_Trade called with incorrect parameters.");
        return 0;
    }
    BuySell *t = new BuySell(q, lua_toboolean(s, 3));
    lua_pushnil(s);
    while (lua_next(s, 4))
    {
        if (!lua_istable(s, -1))
        {
            LOG_WARN("LuaNpc_Trade called with incorrect parameters.");
            t->cancel();
            return 0;
        }
        int v[3];
        for (int i = 0; i < 3; ++i)
        {
            lua_rawgeti(s, -1, i + 1);
            if (!lua_isnumber(s, -1))
            {
                LOG_WARN("LuaNpc_Trade called with incorrect parameters.");
                t->cancel();
                return 0;
            }
            v[i] = lua_tointeger(s, -1);
            lua_pop(s, 1);
        }
        t->registerItem(v[0], v[1], v[2]);
        lua_pop(s, 1);
    }
    t->start(p);
    return 0;
}

LuaScript::LuaScript():
    nbArgs(-1)
{
    mState = luaL_newstate();
    luaL_openlibs(mState);

    // Put some callback functions in the scripting environment.
    static luaL_reg const callbacks[] = {
        { "msg_npc_message",  &LuaMsg_NpcMessage  },
        { "msg_npc_choice",   &LuaMsg_NpcChoice   },
        { "obj_create_npc",   &LuaObj_CreateNpc   },
        { "chr_warp",         &LuaChr_Warp        },
        { "chr_inv_change",   &LuaChr_InvChange   },
        { "chr_inv_count",    &LuaChr_InvCount    },
        { "npc_trade",        &LuaNpc_Trade       },
        { NULL, NULL }
    };
    luaL_register(mState, "tmw", callbacks);

    // Make script object available to callback functions.
    lua_pushlightuserdata(mState, (void *)&registryKey);
    lua_pushlightuserdata(mState, this);
    lua_settable(mState, LUA_REGISTRYINDEX);

    lua_settop(mState, 0);
    loadFile("scripts/libtmw.lua");
}

LuaScript::~LuaScript()
{
    lua_close(mState);
}

void LuaScript::prepare(std::string const &name)
{
    assert(nbArgs == -1);
    lua_getglobal(mState, name.c_str());
    nbArgs = 0;
}

void LuaScript::push(int v)
{
    assert(nbArgs >= 0);
    lua_pushinteger(mState, v);
    ++nbArgs;
}

void LuaScript::push(Thing *v)
{
    assert(nbArgs >= 0);
    lua_pushlightuserdata(mState, v);
    ++nbArgs;
}

int LuaScript::execute()
{
    assert(nbArgs >= 0);
    int res = lua_pcall(mState, nbArgs, 1, 0);
    nbArgs = -1;
    if (res || !(lua_isnil(mState, 1) || lua_isnumber(mState, 1)))
    {
        char const *s = lua_tostring(mState, 1);
        LOG_WARN("Failure while calling Lua function: error=" << res
                 << ", type=" << lua_typename(mState, lua_type(mState, 1))
                 << ", message=" << (s ? s : ""));
        lua_pop(mState, 1);
        return 0;
    }
    res = lua_tointeger(mState, 1);
    lua_pop(mState, 1);
    return res;
}

void LuaScript::load(char const *prog)
{
    int res = luaL_loadstring(mState, prog);

    if (res == LUA_ERRSYNTAX)
    {
        LOG_ERROR("Syntax error while loading Lua script.");
        return;
    }

    // A Lua chunk is like a function, so "execute" it in order to initialize it.
    res = lua_pcall(mState, 0, 0, 0);
    if (res)
    {
        LOG_ERROR("Failure while initializing Lua script: "
                  << lua_tostring(mState, -1));
        lua_settop(mState, 0);
        return;
    }
}

static Script *LuaFactory()
{
    return new LuaScript();
}

struct LuaRegister
{
    LuaRegister() { Script::registerEngine("lua", LuaFactory); }
};

static LuaRegister dummy;