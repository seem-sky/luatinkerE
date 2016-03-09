// lua_tinker.cpp
//
// LuaTinker - Simple and light C++ wrapper for Lua.
//
// Copyright (c) 2005-2007 Kwon-il Lee (zupet@hitel.net)
// 
// please check Licence.txt file for licence and legal issues. 

#include <iostream>
#include "lua_tinker.h"
#include<string>
#if defined(_MSC_VER)
    #define I64_FMT "I64"
#elif defined(__APPLE__) 
    #define I64_FMT "q"
#else
    #define I64_FMT "ll"
#endif

/*---------------------------------------------------------------------------*/ 
/* init                                                                      */ 
/*---------------------------------------------------------------------------*/ 
void lua_tinker::init(lua_State *L)
{
	init_shared_ptr(L);
}

/*---------------------------------------------------------------------------*/
void lua_tinker::init_shared_ptr(lua_State *L)
{
	
	lua_newtable(L);

	lua_pushstring(L, "__name");
	lua_pushstring(L, S_SHARED_PTR_NAME);
	lua_rawset(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcclosure(L, meta_get, 0);
	lua_rawset(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcclosure(L, meta_set, 0);
	lua_rawset(L, -3);

	lua_pushstring(L, "__gc");
	lua_pushcclosure(L, destroyer_shared_ptr, 0);
	lua_rawset(L, -3);

	lua_setglobal(L, S_SHARED_PTR_NAME); //pop table
}

int lua_tinker::destroyer_shared_ptr(lua_State *L)
{
	((UserDataWapper*)lua_touserdata(L, 1))->~UserDataWapper();
	return 0;
}


/*---------------------------------------------------------------------------*/ 
/* excution                                                                  */ 
/*---------------------------------------------------------------------------*/ 
void lua_tinker::dofile(lua_State *L, const char *filename)
{
    lua_pushcclosure(L, on_error, 0);
    int errfunc = lua_gettop(L);

    if(luaL_loadfile(L, filename) == 0)
    {
        lua_pcall(L, 0, 1, errfunc);
    }
    else
    {
        print_error(L, "%s", lua_tostring(L, -1));
    }

    lua_remove(L, errfunc);
    lua_pop(L, 1);
}

/*---------------------------------------------------------------------------*/ 
void lua_tinker::dostring(lua_State *L, const char* buff)
{
    lua_tinker::dobuffer(L, buff, strlen(buff));
}

/*---------------------------------------------------------------------------*/ 
void lua_tinker::dobuffer(lua_State *L, const char* buff, size_t len)
{
    lua_pushcclosure(L, on_error, 0);
    int errfunc = lua_gettop(L);

    if(luaL_loadbuffer(L, buff, len, "lua_tinker::dobuffer()") == 0)
    {
        lua_pcall(L, 0, 1, errfunc);
    }
    else
    {
        print_error(L, "%s", lua_tostring(L, -1));
    }

    lua_remove(L, errfunc);
    lua_pop(L, 1);
}

/*---------------------------------------------------------------------------*/ 
/* debug helpers                                                             */ 
/*---------------------------------------------------------------------------*/ 
static void call_stack(lua_State* L, int n)
{
    lua_Debug ar;
    if(lua_getstack(L, n, &ar) == 1)
    {
        lua_getinfo(L, "nSlu", &ar);

        const char* indent;
        if(n == 0)
        {
            indent = "->\t";
            lua_tinker::print_error(L, "\t<call stack>");
        }
        else
        {
            indent = "\t";
        }

        if(ar.name)
            lua_tinker::print_error(L, "%s%s() : line %d [%s : line %d]", indent, ar.name, ar.currentline, ar.source, ar.linedefined);
        else
            lua_tinker::print_error(L, "%sunknown : line %d [%s : line %d]", indent, ar.currentline, ar.source, ar.linedefined);

        call_stack(L, n+1);
    }
}

/*---------------------------------------------------------------------------*/ 
int lua_tinker::on_error(lua_State *L)
{
    print_error(L, "%s", lua_tostring(L, -1));

    call_stack(L, 0);

    return 0;
}

/*---------------------------------------------------------------------------*/ 
void lua_tinker::print_error(lua_State *L, const char* fmt, ...)
{
    char text[4096];

    va_list args;
    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);

    lua_getglobal(L, "_ALERT");
    if(lua_isfunction(L, -1))
    {
        lua_pushstring(L, text);
        lua_call(L, 1, 0);
    }
    else
    {
        printf("%s\n", text);
        lua_pop(L, 1);
    }
}

/*---------------------------------------------------------------------------*/ 
void lua_tinker::enum_stack(lua_State *L)
{
    int top = lua_gettop(L);
    print_error(L, "%s","----------stack----------");
    print_error(L, "Type:%d", top);
    for(int i=1; i<=lua_gettop(L); ++i)
    {
        switch(lua_type(L, i))
        {
        case LUA_TNIL:
            print_error(L, "\t%s", lua_typename(L, lua_type(L, i)));
            break;
        case LUA_TBOOLEAN:
            print_error(L, "\t%s    %s", lua_typename(L, lua_type(L, i)), lua_toboolean(L, i)?"true":"false");
            break;
        case LUA_TLIGHTUSERDATA:
            print_error(L, "\t%s    0x%08p", lua_typename(L, lua_type(L, i)), lua_topointer(L, i));
            break;
        case LUA_TNUMBER:
            print_error(L, "\t%s    %f", lua_typename(L, lua_type(L, i)), lua_tonumber(L, i));
            break;
        case LUA_TSTRING:
            print_error(L, "\t%s    %s", lua_typename(L, lua_type(L, i)), lua_tostring(L, i));
            break;
        case LUA_TTABLE:
            print_error(L, "\t%s    0x%08p", lua_typename(L, lua_type(L, i)), lua_topointer(L, i));
            break;
        case LUA_TFUNCTION:
            print_error(L, "\t%s()  0x%08p", lua_typename(L, lua_type(L, i)), lua_topointer(L, i));
            break;
        case LUA_TUSERDATA:
            print_error(L, "\t%s    0x%08p", lua_typename(L, lua_type(L, i)), lua_topointer(L, i));
            break;
        case LUA_TTHREAD:
            print_error(L, "\t%s", lua_typename(L, lua_type(L, i)));
            break;
        }
    }
    print_error(L, "%s","-------------------------");
}
 
/*---------------------------------------------------------------------------*/ 
/* _read                                                                      */ 
/*---------------------------------------------------------------------------*/ 
char* lua_tinker::_stack_help<char*>::_read(lua_State *L, int index)
{
    return (char*)lua_tostring(L, index);               
}

const char* lua_tinker::_stack_help<const char*>::_read(lua_State *L, int index)
{
    return (const char*)lua_tostring(L, index);     
}

bool lua_tinker::_stack_help<bool>::_read(lua_State *L, int index)
{
    if(lua_isboolean(L, index))
        return lua_toboolean(L, index) != 0;                
    else
        return lua_tointeger(L, index) != 0;
}

lua_tinker::table lua_tinker::_stack_help<lua_tinker::table>::_read(lua_State *L, int index)
{
    return table(L, index);
}

std::string lua_tinker::_stack_help<std::string>::_read(lua_State *L, int index)
{
	return std::string((const char*)lua_tostring(L, index));
}

std::string lua_tinker::_stack_help<const std::string&>::_read(lua_State *L, int index)
{
	return std::string((const char*)lua_tostring(L, index));
}



/*---------------------------------------------------------------------------*/ 
/* _push                                                                      */ 
/*---------------------------------------------------------------------------*/ 
void lua_tinker::_stack_help<char*>::_push(lua_State *L, char* ret)
{
    lua_pushstring(L, ret);                     
}

void lua_tinker::_stack_help<const char*>::_push(lua_State *L, const char* ret)
{
    lua_pushstring(L, ret);                     
}

void lua_tinker::_stack_help<bool>::_push(lua_State *L, bool ret)
{
    lua_pushboolean(L, ret);                        
}

void lua_tinker::_stack_help<lua_tinker::lua_value*>::_push(lua_State *L, lua_value* ret)
{
    if(ret) ret->to_lua(L); else lua_pushnil(L);    
}

void lua_tinker::_stack_help<lua_tinker::table>::_push(lua_State *L, lua_tinker::table ret)
{
    lua_pushvalue(L, ret.m_obj->m_index);
}

void lua_tinker::_stack_help<std::string>::_push(lua_State *L, std::string ret)
{
	lua_pushlstring(L, ret.data(), ret.size());
}

void lua_tinker::_stack_help<const std::string&>::_push(lua_State *L, const std::string& ret)
{
	lua_pushlstring(L, ret.data(), ret.size());
}

/*---------------------------------------------------------------------------*/ 
/* pop                                                                       */ 
/*---------------------------------------------------------------------------*/ 
template<>
void lua_tinker::pop(lua_State *L)
{
    lua_pop(L, 1);
}

template<>  
lua_tinker::table lua_tinker::pop(lua_State *L)
{
    return table(L, lua_gettop(L));
}

/*---------------------------------------------------------------------------*/ 
/* Tinker Class Helper                                                       */ 
/*---------------------------------------------------------------------------*/ 
static void invoke_parent(lua_State *L)
{
    lua_pushstring(L, "__parent");
    lua_rawget(L, -2);
    if(lua_istable(L,-1))
    {
        lua_pushvalue(L,2);
        lua_rawget(L, -2);
        if(!lua_isnil(L,-1))
        {
            lua_remove(L,-2);
        }
        else
        {
            lua_remove(L, -1);
            invoke_parent(L);
            lua_remove(L,-2);
        }
    }
}

/*---------------------------------------------------------------------------*/ 
int lua_tinker::meta_get(lua_State *L)
{
    lua_getmetatable(L,1);
    lua_pushvalue(L,2);
    lua_rawget(L,-2);

    if(lua_isuserdata(L,-1) != 0)
    {
        user2type<var_base*>(L,-1)->get(L);
        lua_remove(L, -2);
    }
    else if(lua_isnil(L,-1))
    {
        lua_remove(L,-1);
        invoke_parent(L);
        if(lua_isuserdata(L,-1))
        {
            user2type<var_base*>(L,-1)->get(L);
            lua_remove(L, -2);
        }
        else if(lua_isnil(L,-1))
        {
            lua_pushfstring(L, "can't find '%s' class variable. (forgot registering class variable ?)", lua_tostring(L, 2));
            lua_error(L);
        }
    } 
    lua_remove(L,-2);

    return 1;
}

/*---------------------------------------------------------------------------*/ 
int lua_tinker::meta_set(lua_State *L)
{
    lua_getmetatable(L,1);
    lua_pushvalue(L,2);
    lua_rawget(L,-2);

    if(lua_isuserdata(L,-1))
    {
        user2type<var_base*>(L,-1)->set(L);
    }
    else if(lua_isnil(L, -1))
    {
        lua_remove(L,-1);
        lua_pushvalue(L,2);
        lua_pushvalue(L,4);
        invoke_parent(L);
        if(lua_isuserdata(L,-1))
        {
            user2type<var_base*>(L,-1)->set(L);
        }
        else if(lua_isnil(L,-1))
        {
            lua_pushfstring(L, "can't find '%s' class variable. (forgot registering class variable ?)", lua_tostring(L, 2));
            lua_error(L);
        }
    }
    lua_settop(L, 3);
    return 0;
}

/*---------------------------------------------------------------------------*/ 
void lua_tinker::push_meta(lua_State *L, const char* name)
{
    lua_getglobal(L, name);
}

/*---------------------------------------------------------------------------*/ 
/* table object on stack                                                     */ 
/*---------------------------------------------------------------------------*/ 
lua_tinker::table_obj::table_obj(lua_State* L, int index)
    :m_L(L)
    ,m_index(index)
    ,m_ref(0)
{
    if(lua_isnil(m_L, m_index))
    {
        m_pointer = NULL;
        lua_remove(m_L, m_index);
    }
    else
    {
        m_pointer = lua_topointer(m_L, m_index);
    }
}

lua_tinker::table_obj::~table_obj()
{
    if(validate())
    {
        lua_remove(m_L, m_index);
    }
}

void lua_tinker::table_obj::inc_ref()
{
    ++m_ref;
}

void lua_tinker::table_obj::dec_ref()
{
    if(--m_ref == 0)
        delete this;
}

bool lua_tinker::table_obj::validate()
{
    if(m_pointer != NULL)
    {
        if(m_pointer == lua_topointer(m_L, m_index))
        {
            return true;
        }
        else
        {
            int top = lua_gettop(m_L);

            for(int i=1; i<=top; ++i)
            {
                if(m_pointer == lua_topointer(m_L, i))
                {
                    m_index = i;
                    return true;
                }
            }

            m_pointer = NULL;
            return false;
        }
    }
    else
    {
        return false;
    }
}

/*---------------------------------------------------------------------------*/ 
/* Table Object Holder                                                       */ 
/*---------------------------------------------------------------------------*/ 
lua_tinker::table::table(lua_State* L)
{
    lua_newtable(L);

    m_obj = new table_obj(L, lua_gettop(L));

    m_obj->inc_ref();
}

lua_tinker::table::table(lua_State* L, const char* name)
{
    lua_getglobal(L, name);

    if(lua_istable(L, -1) == 0)
    {
        lua_pop(L, 1);

        lua_newtable(L);
        lua_setglobal(L, name);
        lua_getglobal(L, name);
    }

    m_obj = new table_obj(L, lua_gettop(L));

    m_obj->inc_ref();
}

lua_tinker::table::table(lua_State* L, int index)
{
    if(index < 0)
    {
        index = lua_gettop(L) + index + 1;
    }

    m_obj = new table_obj(L, index);

    m_obj->inc_ref();
}

lua_tinker::table::table(const table& input)
{
    m_obj = input.m_obj;

    m_obj->inc_ref();
}

lua_tinker::table::~table()
{
    m_obj->dec_ref();
}

/*---------------------------------------------------------------------------*/ 