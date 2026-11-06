#pragma once

#include <memory>
#include <new>

// COP_ENGINE_PATCH: COP-ENG-158
// Preserve the Luabind pair iterator's snapshot semantics, but give its native
// container an owner that Lua can finalize. Do not iterate the live ALife map.
// The Luabind conversion-policy declarations are provided by pch_script.h.
namespace luabind::detail
{
    template<typename Container>
    struct alife_iterator_state
    {
        using Iter = typename Container::const_iterator;

        std::shared_ptr<Container> container;
        Iter start, end;

        static int destroy(lua_State* L)
        {
            auto* state = static_cast<alife_iterator_state*>(lua_touserdata(L, 1));
            state->~alife_iterator_state();
            return 0;
        }

        static void push_metatable(lua_State* L)
        {
            // Each C++ container type and each Lua state need their own table.
            static char key;
            lua_pushlightuserdata(L, &key);
            lua_rawget(L, LUA_REGISTRYINDEX);
            if (!lua_isnil(L, -1))
                return;

            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushcfunction(L, destroy);
            lua_setfield(L, -2, "__gc");
            lua_pushboolean(L, 0);
            lua_setfield(L, -2, "__metatable");
            lua_pushlightuserdata(L, &key);
            lua_pushvalue(L, -2);
            lua_rawset(L, LUA_REGISTRYINDEX);
        }

        static int step(lua_State* L)
        {
            auto* state = static_cast<alife_iterator_state*>(lua_touserdata(L, lua_upvalueindex(1)));
            if (!state || !state->container || state->start == state->end)
            {
                lua_pushnil(L);
                return 1;
            }

            convert_to_lua(L, state->start->first);
            convert_to_lua(L, state->start->second);
            ++state->start;
            return 2;
        }

        static void push(lua_State* L, const Container& source)
        {
            // Lua allocation can longjmp. Install __gc before owning a native
            // map, and keep no temporary shared_ptr across subsequent Lua calls.
            push_metatable(L);
            void* storage = lua_newuserdata(L, sizeof(alife_iterator_state));
            auto* state = new (storage) alife_iterator_state();
            lua_pushvalue(L, -2);
            lua_setmetatable(L, -2);
            lua_remove(L, -2);

            state->container = std::make_shared<Container>(source);
            state->start = state->container->begin();
            state->end = state->container->end();
            lua_pushcclosure(L, step, 1);
        }
    };

    struct alife_iterator_converter
    {
        template<typename T>
        void apply(lua_State* L, T& source)
        {
            alife_iterator_state<T>::push(L, source);
        }

        template<typename T>
        void apply(lua_State* L, const T& source)
        {
            alife_iterator_state<T>::push(L, source);
        }
    };

    struct alife_iterator_policy : conversion_policy<0>
    {
        static void precall(lua_State*, const index_map&) {}
        static void postcall(lua_State*, const index_map&) {}

        template<typename T, Direction>
        struct generate_converter
        {
            using type = alife_iterator_converter;
        };
    };
}

namespace luabind
{
    namespace
    {
        detail::policy_cons<detail::alife_iterator_policy> return_alife_pair_iterator;
    }
}
