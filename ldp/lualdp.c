#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "ldp.h"

static int lualdp_interface_open(lua_State *lua)
{
  const char *dev = luaL_checkstring(lua, 1);
  int numinq = lua_tointeger(lua, 2);
  int numoutq = lua_tointeger(lua, 3);
  struct ldp_interface *intf = ldp_interface_open(dev, numinq, numoutq);
  if (intf == NULL)
  {
    lua_pushnil(lua);
  }
  else
  {
    lua_pushlightuserdata(lua, intf);
  }
  return 1;
}

static int lualdp_get_inq(lua_State *lua)
{
  struct ldp_interface *intf = lua_touserdata(lua, 1);
  int qid = lua_tointeger(lua, 2);
  if (qid <= 0 || qid > intf->num_inq)
  {
    lua_pushnil(lua);
  }
  else
  {
    lua_pushlightuserdata(lua, intf->inq[qid - 1]);
  }
  return 1;
}

static int lualdp_get_outq(lua_State *lua)
{
  struct ldp_interface *intf = lua_touserdata(lua, 1);
  int qid = lua_tointeger(lua, 2);
  if (qid <= 0 || qid > intf->num_outq)
  {
    lua_pushnil(lua);
  }
  else
  {
    lua_pushlightuserdata(lua, intf->outq[qid - 1]);
  }
  return 1;
}

static int lualdp_inq_nextpkts(lua_State *lua)
{
  struct ldp_in_queue *inq = lua_touserdata(lua, 1);
  int numpkts = lua_tointeger(lua, 2);
  struct ldp_packet pkts[numpkts];
  int ret;
  int i;
  //usleep(10000);
  ret = ldp_in_nextpkts(inq, pkts, numpkts);
  if (ret <= 0)
  {
    lua_newtable(lua);
    return 1;
  }
  lua_newtable(lua);
  for (i = 0; i < ret; i++)
  {
    lua_pushinteger(lua, i+1);
    lua_pushlstring(lua, pkts[i].data, pkts[i].sz);
    lua_settable(lua, -3);
  }
  ldp_in_deallocate_some(inq, pkts, ret);
  return 1;
}

static int lualdp_outq_inject(lua_State *lua)
{
  struct ldp_out_queue *outq = lua_touserdata(lua, 1);
  luaL_checktype(lua, 2, LUA_TTABLE);
  int len = lua_objlen(lua, 2);
  int i;
  int top = lua_gettop(lua);
  int ret;
  struct ldp_packet pkts[len];
  for (i = 0; i < len; i++)
  {
    size_t pktlen;
    const void *data;
    lua_pushinteger(lua, i+1);
    lua_gettable(lua, top-0);
    data = lua_tolstring(lua, -1, &pktlen);
    pkts[i].sz = pktlen;
    pkts[i].data = (void*)data;
  }
  ret = ldp_out_inject(outq, pkts, len);
  lua_settop(lua, top);
  lua_pushinteger(lua, ret);
  return 1;
}

static int lualdp_inq_eof(lua_State *lua)
{
  struct ldp_in_queue *inq = lua_touserdata(lua, 1);
  lua_pushboolean(lua, ldp_in_eof(inq));
  return 1;
}

static int lualdp_inq_poll(lua_State *lua)
{
  struct ldp_in_queue *inq = lua_touserdata(lua, 1);
  int timeout_usec = lua_tointeger(lua, 2);
  lua_pushinteger(lua, ldp_in_poll(inq, timeout_usec));
  return 1;
}


static int lualdp_outq_txsync(lua_State *lua)
{
  struct ldp_out_queue *outq = lua_touserdata(lua, 1);
  ldp_out_txsync(outq);
  lua_pushboolean(lua, 1);
  return 1;
}

static int lualdp_interface_close(lua_State *lua)
{
  struct ldp_interface *intf = lua_touserdata(lua, 1);
  ldp_interface_close(intf);
  return 1;
}

int luaopen_lualdp(lua_State *lua);

int luaopen_lualdp(lua_State *lua)
{
  static const luaL_Reg ldp_lib[] = {
    {"interface_open", lualdp_interface_open},
    {"interface_close", lualdp_interface_close},
    {"outq_txsync", lualdp_outq_txsync},
    {"inq_eof", lualdp_inq_eof},
    {"inq_poll", lualdp_inq_poll},
    {"inq_nextpkts", lualdp_inq_nextpkts},
    {"outq_inject", lualdp_outq_inject},
    {"get_inq", lualdp_get_inq},
    {"get_outq", lualdp_get_outq},
    {NULL, NULL}};

  luaL_newlib(lua, ldp_lib);
  return 1;
}
