#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <memory>

#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>

#include <tbb/concurrent_hash_map.h>

#include <i01_core/macro.hpp>
#include <i01_core/Singleton.hpp>
#include <i01_core/Time.hpp>
#include <i01_core/Lock.hpp>

namespace i01 { namespace core {

typedef std::map<std::string, std::string> ConfigStateBase;

class ConfigState;
using ConfigStateSharedPtr = std::shared_ptr<ConfigState>;

/// Mapping of keys to values.  Use `get<T>(key)` to obtain a `Maybe T`
/// containing the desired key (or `boost::none` if not found).  The user
/// should never have to write to this class.
class ConfigState : public ConfigStateBase
                  , public std::enable_shared_from_this<ConfigState> {
public:
    /// Create ConfigState objects with shared ownership.
    template <typename... Args>
    static std::shared_ptr<ConfigState> create( Args&& ... args )
    {
        auto cs = std::make_shared<ConfigState>(std::forward<Args>(args)...);
        if (cs) {
            cs->version(0);
        }
        return cs;
    }

    typedef key_type prefix_type;
    const prefix_type& prefix() const { return m_prefix; }

    typedef std::uint64_t version_type;
    const version_type& version() const { return m_version; }

    /// Return true if the user-specified key exists and copy it into
    /// the user-specified destination; or return false otherwise and leave
    /// destination unmodified.
    //  This is a useful pattern for setting default values:
    //  <code>
    //      int limit = 0;
    //      if (cs_p->get<int>("limit", limit))
    //          std::cout << "custom limit = " << limit << std::endl;
    //      else std::cout << "using default limit." << std::endl;
    //  </code>
    template <typename T>
    bool get(const key_type& key, T& destination) const noexcept;
    /// Return a value corresponding to the user-specified key, or
    /// boost::none if the key does not exist.
    //  This is a useful pattern when no default value is desired:
    //  <code>
    //      if (int limit = cs_p->get<int>("limit")) /* relies on operator(bool) */
    //        std::cout << "limit found = " << limit << std::endl;
    //      else std::cerr << "crap.  no limit found." << std::endl;
    //  </code>
    template <typename T>
    const boost::optional<T> get(const key_type& key) const noexcept;
    /// Same as boost::optional version of `get(key)`, except returns
    /// default_value if no value is found.
    template <typename T>
    const T get_or_default(const key_type& key, const T default_value) const noexcept;
    /// Return a map of keys that matches the prefix, with the prefix removed.
    /// For example: if prefix is "oe.sessions.", will return a
    /// ConfigStateBase of "X" -> Y if "X" was "oe.sessions.X" originally.
    std::shared_ptr<ConfigState> copy_prefix_domain(const key_type& prefix) const noexcept;
    /// Returns a set of key prefixes in the current level (for use with
    /// copy_prefix_domain).  I probably should have used a tree to store
    /// configs, ugh.
    std::set<key_type> get_key_prefix_set(const char delimiter = '.') const;
private:
    using ConfigStateBase::ConfigStateBase;

    std::uint64_t m_version;
    std::string m_prefix;
    void prefix(const prefix_type& new_prefix) { m_prefix = new_prefix; }
    void version(const version_type& new_version) { m_version = new_version; }
    friend class Config;
};

template <typename T>
bool ConfigState::get(const key_type& key, T& destination) const noexcept {
    const auto it = find(key);
    if (it != cend()) {
        try {
            destination = boost::lexical_cast<T>(it->second);
            return true;
        } catch (const boost::bad_lexical_cast&) {
            return false;
        }
    }
    return false;
}

template <>
bool ConfigState::get<ConfigState::mapped_type>(const key_type& key, mapped_type& destination) const noexcept;

template <typename T>
const boost::optional<T> ConfigState::get(const key_type& key) const noexcept {
    T result;
    if (get(key, result))
        return result;
    else
        return boost::none;
}

template <typename T>
const T ConfigState::get_or_default(const key_type& key, const T default_value) const noexcept {
    if (boost::optional<T> result = get<T>(key))
        return *result;
    return std::move(default_value);
}

class ConfigListener;
///
class Config : public Singleton<Config> {
public:
    typedef ConfigState storage_type;
    typedef storage_type::size_type size_type;
    typedef storage_type::key_type key_type;
    typedef storage_type::mapped_type mapped_type;
    typedef storage_type::value_type value_type;
    typedef storage_type::version_type version_type;
    typedef RecursiveMutex mutex_type;

    /* Accessors: */

    const std::shared_ptr<const ConfigState> get_shared_state() const { return m_storage; }
    version_type current_version() const { if (m_storage) return m_storage->version(); else return 0; }

    /* Mutators: */

    Config() : m_storage(storage_type::create()) { m_storage->version(0); }

    bool subscribe(ConfigListener* lp) {
        LockGuard<mutex_type> lock(m_mutex);
        if (!lp)
            return false;
        return m_listeners.insert(lp).second;
    }
    bool unsubscribe(ConfigListener* lp) {
        LockGuard<mutex_type> lock(m_mutex);
        if (!lp)
            return false;
        return m_listeners.erase(lp) > 0;
    }

    int copy_key(const std::string &src_key, const std::string &dst_domain, bool overwrite = false, const char delimiter = '.');

    void load_strings(const std::map<std::string, std::string> &str);
    void load_environ(const std::string& prefix = "I01_");
    void load_variables_map(const boost::program_options::variables_map& vm);
    void load_lua_file(const std::string& filename, const std::string& global_name = "conf");

    void reset() { update(storage_type::create()); }

protected:
    void update(const std::shared_ptr<storage_type>& new_state);

    std::shared_ptr<storage_type> m_storage;
    std::set<ConfigListener*> m_listeners;
    mutable mutex_type m_mutex;

    friend class Singleton<Config>;
};

class ConfigListener {
    public:
        ConfigListener() { }
        virtual ~ConfigListener() { }

        bool subscribe() { return Config::instance().subscribe(this); }
        bool unsubscribe() { return Config::instance().unsubscribe(this); }
        /// Called upon an "atomic" configuration update.  The receiver may
        /// want to use `std::set_difference` or simply re-read relevant
        /// configuration keys from new_state.
        virtual void on_config_update(const Config::storage_type& old_state, const Config::storage_type& new_state) noexcept = 0;
};


} }
