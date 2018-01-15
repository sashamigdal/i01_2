#include <string>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "lua.hpp"

#include <i01_core/Config.hpp>
#include <i01_core/Environment.hpp>

namespace i01 { namespace core {

static void setLuaPath( lua_State* L, const std::string& path )
{
        lua_getglobal( L, "package" );
        lua_getfield( L, -1, "path" ); // get field "path" from table at top of stack (-1)
        std::string cur_path = lua_tostring( L, -1 ); // grab path string from top of stack
        cur_path = path + ";" + cur_path;
        lua_pop( L, 1 ); // get rid of the string on the stack we just pushed on line 5
        lua_pushstring( L, cur_path.c_str() ); // push the new one
        lua_setfield( L, -2, "path" ); // set the field "path" in table at -2 with value at top of stack
        lua_pop( L, 1 ); // get rid of package table from top of stack
}

/* public: */

template <>
bool ConfigState::get<ConfigState::mapped_type>(const key_type& key, mapped_type& destination) const noexcept
{
    const auto it = find(key);
    if (it != cend()) {
        destination = it->second;
        return true;
    }
    return false;
}

std::shared_ptr<ConfigState> ConfigState::copy_prefix_domain(const key_type& prefix_) const noexcept
{
    std::shared_ptr<ConfigState> cs = std::make_shared<ConfigState>();
    cs->version(version());
    std::string new_prefix = prefix() + prefix_;
    cs->prefix(new_prefix);
    for (auto it = begin(); it != end(); ++it) {
        if (0 == it->first.compare(0,prefix_.size(),prefix_))
            cs->emplace(std::make_pair(
                        it->first.substr(prefix_.size()),
                        it->second));
    }
    return cs;
}

std::set<ConfigState::key_type> ConfigState::get_key_prefix_set(const char delimiter) const
{
    std::set<key_type> s;
    for (auto it = begin(); it != end(); ++it) {
        s.insert(it->first.substr(0, it->first.find(delimiter)));
    }
    return s;
}

int Config::copy_key(const std::string &src_key, const std::string &dst_domain, bool overwrite, const char delimiter)
{
    int ret = 0;
    LockGuard<mutex_type> lock(m_mutex);

    auto cs = get_shared_state();
    auto newcs = ConfigState::create(*m_storage);

    // auto sfx = cs->get_key_suffix_set();
    auto it = cs->find(src_key);
    if (it == cs->end()) {
        return ret;
    }

    auto k = src_key;
    auto p = k.find_last_of(delimiter);
    if (p != std::string::npos) {
        k = k.substr(p+1);
    }
    auto dstk = dst_domain;
    if (dstk[dst_domain.size()-1] == delimiter) {
        dstk += k;
    } else {
        dstk += delimiter + k;
    }

    if (!overwrite) {
        if (cs->find(dstk) != cs->end()) {
            return ret;
        }
    }
    (*newcs)[dstk] = it->second;
    ret++;

    update(newcs);

    return ret;
}

void Config::load_strings(const std::map<std::string, std::string> &strs)
{
    LockGuard<mutex_type> lock(m_mutex);
    auto cs = ConfigState::create(*m_storage);

    for (auto kv : strs) {
        (*cs)[kv.first] = kv.second;
    }

    update(cs);
}

void Config::load_environ(const std::string& prefix)
{
    LockGuard<mutex_type> lock(m_mutex);
    Environment e;
    e.load();
    auto cs = ConfigState::create(*m_storage);

    for (const auto& kv : e) {
        if (kv.first.compare(0, prefix.length(), prefix) == 0) {
            (*cs)[kv.first.substr(prefix.length(), std::string::npos)] = kv.second;
        }
    }
    update(cs);
}

static std::vector<Config::mapped_type> any_to_vec_strings(const boost::any& val) noexcept
{
    // TODO: Boost 1.56.0 introduced try_lexical_convert.
    //       Use that instead once we upgrade Boost.
    if (auto v1 = boost::any_cast<std::string>(&val))
    { return std::vector<std::string>{*v1}; }
    else if (auto v2 = boost::any_cast<bool>(&val))
    { return std::vector<std::string>{(*v2 ? "true" : "false")}; }
    else if (auto v3 = boost::any_cast<int>(&val))
    { return std::vector<std::string>{boost::lexical_cast<std::string>(*v3)}; }
    else if (auto v4 = boost::any_cast<double>(&val))
    { return std::vector<std::string>{boost::lexical_cast<std::string>(*v4)}; }
    else if (auto v5 = boost::any_cast<std::vector<std::string>>(&val))
    { return (*v5); }
    else if (auto v6 = boost::any_cast<std::vector<bool>>(&val))
    {
        std::vector<std::string> r;
        for (auto b : *v6) {
            r.push_back(b ? "true" : "false");
        }
        return r;
    }
    else if (auto v7 = boost::any_cast<std::vector<int>>(&val))
    {
        std::vector<std::string> r;
        for (auto i : *v7) {
            r.push_back(boost::lexical_cast<std::string>(i));
        }
        return r;
    }
    else if (auto v8 = boost::any_cast<std::vector<double>>(&val))
    {
        std::vector<std::string> r;
        for (auto d : *v8) {
            r.push_back(boost::lexical_cast<std::string>(d));
        }
        return r;
    }
    else { return std::vector<std::string>(); }
}

void Config::load_variables_map(const boost::program_options::variables_map& vm)
{
    i01::core::LockGuard<mutex_type> lock(m_mutex);
    auto cs = ConfigState::create(*m_storage);
    for (const auto& kv : vm) {
        auto sv = any_to_vec_strings(kv.second.value());
        if (sv.empty())
            continue;
        else if (sv.size() == 1)
            (*cs)[kv.first] = sv[0];
        else if (sv.size() > 1) {
            int i = 1;
            for (const auto& s : sv) {
                (*cs)[kv.first + "." + std::to_string(i)] = s;
                ++i;
            }
        }
    }
    update(cs);
}

namespace {

static bool flatten_luatable(lua_State *L, const std::string& scope, ConfigStateBase& kvmap, bool top_level = false)
{
    bool ret = true;
    for (::lua_pushnil(L); ::lua_next(L, -2) != 0; ::lua_pop(L, 1))
    {
        std::string child_scope;
        if (!top_level)
            child_scope = scope;
        int t1 = ::lua_type(L, -2);
        switch (t1)
        {
            // valid key types:
            case LUA_TBOOLEAN:
                if (!child_scope.empty())
                    child_scope += ".";
                child_scope += ( ::lua_toboolean(L, -2) ? "true" : "false" );
                break;
            case LUA_TNUMBER:
                if (!child_scope.empty())
                    child_scope += ".";
                child_scope += ::lua_tostring(L, -2);
                break;
            case LUA_TSTRING:
                if (!child_scope.empty())
                    child_scope += ".";
                child_scope += ::lua_tostring(L, -2);
                break;
            // invalid key types:
            case LUA_TNONE:
            case LUA_TNIL:
            case LUA_TLIGHTUSERDATA:
            case LUA_TTABLE:
            case LUA_TFUNCTION:
            case LUA_TUSERDATA:
            case LUA_TTHREAD:
            default: {
                // Unhandled type...
                std::cerr << "Config::load_lua_file flatten_luatable unexpected key type: " << scope << " :: " << lua_typename(L, t1) << std::endl;
                return false;
            } /* break; */
        }

        int t2 = lua_type(L, -1);
        switch (t2)
        {
            // valid value types:
            case LUA_TBOOLEAN:
                kvmap[child_scope] = ::lua_toboolean(L, -1) ? "true" : "false";
                break;
            case LUA_TNUMBER:
                kvmap[child_scope] = ::lua_tostring(L, -1);
                break;
            case LUA_TSTRING:
                kvmap[child_scope] = ::lua_tostring(L, -1);
                break;
            case LUA_TNIL:
                // nil value means erase from map:
                kvmap.erase(child_scope);
                break;
            case LUA_TTABLE:
                ret &= flatten_luatable(L, child_scope, kvmap);
                break;
            // invalid key types:
            case LUA_TNONE:
            case LUA_TLIGHTUSERDATA:
            case LUA_TFUNCTION:
            case LUA_TUSERDATA:
            case LUA_TTHREAD:
            default: {
                // Unhandled type...
                std::cerr << "Config::load_lua_file flatten_luatable unexpected value type: " << child_scope << " :: " << lua_typename(L, t2) << std::endl;
                return false;
            } /* break; */
        }

    }
    return ret;
}

/// Exports (flattened) config map from kvmap as a new Lua table named scope.
static bool export_to_luatable(lua_State *L, const std::string& scope, const ConfigStateBase& kvmap)
{
    bool ret = true;
    ::lua_newtable(L);
    for (const auto& kv : kvmap) {
        ::lua_pushstring(L, kv.first.c_str());
        ::lua_pushstring(L, kv.second.c_str());
        ::lua_settable(L, -3);
    }
    ::lua_setglobal(L, scope.c_str());
    return ret;
}
} /* namespace */

void Config::load_lua_file(const std::string& filename, const std::string& global_name)
{
    i01::core::LockGuard<mutex_type> lock(m_mutex);
    // See http://www.lua.org/pil/25.html + Lua 5.2 reference manual
    ::lua_State *L = ::luaL_newstate();
    if (L == nullptr)
    {
        std::cerr << "Config::load_lua_file luaL_newstate() failed." << std::endl;
        return;
    }
    ::luaL_openlibs(L);
    boost::filesystem::path parent(boost::filesystem::path(filename).parent_path());
    setLuaPath(L, ((parent / "?") / "init.lua").c_str());
    setLuaPath(L, (parent / "?.lua").c_str());
    export_to_luatable(L, ("current_" + global_name).c_str(), *m_storage);
    if (::luaL_loadfile(L, filename.c_str()) || lua_pcall(L, 0, 0, 0))
    {
        std::cerr << "Config::load_lua_file luaL_loadfile() failed: " << ::lua_tostring(L, -1) << std::endl;
        lua_close(L);
        return;
    }
    ::lua_getglobal(L, global_name.c_str());
    if (!lua_istable(L, -1))
    {
        std::cerr << "Config::load_lua_file lua_istable() returned false." << std::endl;
        lua_close(L);
        return;
    }
    auto cs = ConfigState::create(*m_storage);
    if (flatten_luatable(L, global_name, *cs, true))
    { update(cs); }
    ::lua_close(L);
}

void Config::update(const std::shared_ptr<storage_type>& new_state) {
    if (new_state == nullptr)
        return;
    i01::core::LockGuard<mutex_type> lock(m_mutex);
    if (!m_storage)
        m_storage = ConfigState::create();
    if ((*m_storage) == (*new_state))
        return;
    auto old_state = m_storage;
    new_state->version(m_storage->version() + 1);
    m_storage = new_state;
    for (auto it = m_listeners.begin(); it != m_listeners.end(); ) {
        if (*it) {
            (*it)->on_config_update(*old_state, *m_storage);
            ++it;
        } else {
            m_listeners.erase(it++);
        }
    }
}

} }
