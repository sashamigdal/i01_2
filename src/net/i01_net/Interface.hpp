#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <algorithm>

#include <i01_core/Lock.hpp>

namespace i01 { namespace net {

class Interface {
public:
    Interface(const char* ifname);
    virtual ~Interface();

    const std::string& name() const
    { return m_if_name; }
    unsigned int index() const
    { return m_if_index; }

    static std::vector<Interface>& list();
    static const std::string get_name_from_address(const std::string& address_);
protected:
    typedef i01::core::RecursiveMutex InterfaceLock;

    /* POSIX interface properties: */

    const std::string   m_if_name;
    const unsigned int  m_if_index;

    static InterfaceLock s_mutex;
    static std::vector<Interface> s_interfaces;
private:
    // Do not allow default construction of interfaces, i.e. interfaces
    // without a name and index.
    Interface() = delete;
};



} }
