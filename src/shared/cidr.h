/*
Copyright (C) 2014 - 2020 Eaton

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/// @file cidr.h
/// @brief Classes for manipulating with CIDR addresses
/// @author Tomas Halman <TomasHalman@Eaton.com>

/// Example:
/// #include "cidr.h"
///
/// CIDRList list;
/// CIDRAddress addr;
///
/// // add two networks
/// list.add("10.20.30.16/30");
/// list.add("10.21.31.0/24");
///
/// // exclude 10.21.31.128 - 255
/// list.exclude("10.21.31.128/25");
/// // but include 10.21.31.192 - 199 (10.21.31.192/29)
/// list.exclude("10.21.31.192/29");
///
/// addr.invalidate(); // this is not necessary, object is created as invalid address 0.0.0.0
/// while( list.next(addr) ) {
///   // next() increments the address and returns true until we are in the address scope
///   std::cout << addr << "\n";
/// }


#pragma once

#include <iostream>
extern "C" {
#include <libcidr.h>
}
#include <string>
#include <vector>

namespace shared {

/// @enum CIDROptions
/// Formating options for CIDRAddress
enum CIDROptions
{
    CIDR_AUTO_PREFIX,
    CIDR_WITH_PREFIX,
    CIDR_WITHOUT_PREFIX
};

/// @class CIDRAddress
///
/// Provide class for manipulating addresses in CIDR format
///
/// This class provides methods for manipulating netwotk addresses in CIDR format (10.0.0.3/32). IPv6 is not fully
/// supported. This class encapsulates libcidr library and add some features for easy use in c++.
class CIDRAddress
{
public:
    /// Creates new CIDRAddress without any specific ip address.
    ///
    /// The address is set to NULL i. e. the .valid() method says false. Lot of methods does nothing in this object
    /// state. You should set real value to this object before continuing.
    CIDRAddress();

    /// Creates new object with specified network.
    /// @param string representing ip address. The address is set to accorting the parameter. Paremeter can be IP
    /// address or network address like
    ///   - "10.0.0.1"
    ///   - "1.2.3.0/24"
    ///   - "::1"
    CIDRAddress(const std::string& address);

    /// Creates new object with specified network.
    /// @param string representing ip address. The address is set to accorting the parameter. Paremeters can be
    ///   - "10.0.0.1","32"
    ///   - "1.2.3.0/24","8"
    /// @param string network prefix
    CIDRAddress(const std::string& address, const std::string& prefix);

    /// Creates new object with specified network.
    /// @param string representing ip address. The address is set to accorting the parameter. Paremeters can be
    ///   - "10.0.0.1",32
    ///   - "1.2.3.0/24", 8
    ///   - "::1",128
    /// @param unsigned int network prefix
    CIDRAddress(const std::string& address, const unsigned int prefix);

    /// Creates new object from in_addr.
    ///
    /// The address is set to accorting the parameter.
    ///
    /// @param structure with ip address
    CIDRAddress(const struct in_addr* address);
    CIDRAddress(const struct in6_addr* address);
    CIDRAddress(const struct sockaddr* address);

    /// Creates new object with specified network.
    /// @param reference to other CIDRAddress object.
    CIDRAddress(const CIDRAddress& address);

    /// Creates new object with specified network (move constructor).
    /// @param reference to other CIDRAddress object.
    CIDRAddress(CIDRAddress&& other);

    /// increases IPv4 address of 1.
    ///
    /// If object if IPv4 valid address (network or host) address is incremented by 1.
    /// If object is a network address, after incrementing it is changed to host address
    /// Expected result are for example:
    /// - 10.0.3.4 => 10.0.3.5
    /// - 10.0.3.0/24 => 10.0.3.1
    /// - 10.0.3.255 => 10.0.4.0
    /// - 255.255.255.255 => 0.0.0.0
    /// - ::1 => ::2
    CIDRAddress& operator++(); // prefix++

    /// decreases IPv4 address of 1.
    /// @see operator++()
    CIDRAddress& operator--();

    /// increases IPv4 address of 1.
    /// @see operator++()
    CIDRAddress operator++(int); // suffix++

    /// decreases IPv4 address of 1.
    /// @see operator++()
    CIDRAddress operator--(int);

    /// set CIDRAddress from string.
    /// @see set(const std::string text)
    ///
    /// Overloaded operator = allows to set IP address from string.
    ///     CIDRAddress A;
    ///     A = "127.0.0.1/32";
    CIDRAddress& operator=(const std::string& address);

    /// set CIDRAddress from another CIDRAddress.
    /// @see set(const CIDRAddress& address)
    ///
    /// Overloaded operator = allows to set IP address from another one.
    ///     CIDRAddress B("127.0.0.1"),A;
    ///     A = B;
    CIDRAddress& operator=(const CIDRAddress& address);
    CIDRAddress& operator=(CIDRAddress&& address);

    /// compare two CIDRAddress.
    /// @see compare(const CIDRAddress& address)
    ///
    /// Overloaded operator > allows to compare two IP addresses. For comparing IPv6 with IPv4, see compare().
    ///
    /// Addresses are compared without prefix consideration (i.e. 10.0.0.0/8 > 10.0.0.0/32, returns false because here
    /// they are equal).
    bool operator>(const CIDRAddress& a2) const;

    /// compare two CIDRAddress.
    /// @see operator>(const CIDRAddress& a2);
    bool operator<(const CIDRAddress& a2) const;

    /// compare two CIDRAddress.
    ///
    /// Compares two IP addresses (networks), including prefix length.
    bool operator==(const CIDRAddress& a2) const;
    bool operator!=(const CIDRAddress& a2) const;

    /// compare CIDRAddress with string.
    ///
    /// Compares two IP addresses (networks), including prefix length.
    bool operator==(const std::string& a2) const;
    bool operator!=(const std::string& a2) const;

    /// address type
    /// @return int 0 - invalid, 4 - ipv4, 6 - ipv6
    int protocol() const;

    /// prefix of CIDRAddress.
    ///
    /// Method returns prefix or -1 on error (invalid address).
    int prefix() const;

    /// netmask of CIDRAddress.
    ///
    /// Method returns netmask or "" on error (invalid address, IPv6).
    std::string netmask() const;

    /// test if this addres can be netmask.
    ///
    /// Method returns true if address is netmask. Make sense for IPv4. for example
    /// * CIDRAddress("255.255.0.3").isNetmask() == false
    /// * CIDRAddress("255.128.0.0").isNetmask() == true
    bool isNetmask() const;

    /// set IP address to invalid (You can imagine this address as 0.0.0.0 or ::)
    void invalidate();

    /// check itselfs for validity.
    /// @return true for valid address
    bool valid() const;

    /// checks if network fully contains the other.
    /// @return true if address is in block.
    ///
    /// You can check wheter Your host or network belongs to particular network.
    ///     CIDRAddress("10.0.0.0/24").contains( CIDRAddress("10.0.0.128/25") ); // true
    ///     CIDRAddress("10.0.0.0/24").contains( CIDRAddress("10.0.0.0/24") ); // true
    ///     CIDRAddress("10.0.0.0/25").contains( CIDRAddress("10.0.0.0/24") ); // false
    bool contains(const CIDRAddress& address) const;

    /// checks if the network or host is fully contained in the other.
    /// @see valid();
    bool in(const CIDRAddress& address) const;

    /// checks if networks or hosts are the same
    /// @see operator==();
    ///
    /// Two CIDRAddress are compared including network prefix.
    bool equals(const CIDRAddress& address) const;

    /// returns the minimal host address from network
    ///
    /// In case of usual prefix, the returned address is as expected
    ///     - 10.0.0.0/24 => 10.0.0.1
    /// Value 10.0.0.0 is considered as network address, not host. But in case of long prefix (31,32 for IPv4 or 127,128
    /// for IPv6) it is not a network segment, it is a host or point-to-point link. So
    ///     - 10.0.0.0/31 => 10.0.0.0
    CIDRAddress hostMin() const;

    /// returns the maximal host address from network
    /// @see hostMin()
    ///
    /// Returned address is the last one before the broadcast (10.0.0.0/24 => 10.0.0.254).
    /// In case of long prefix there is not a broadcast. The reason is described in hostMin()
    CIDRAddress hostMax() const;

    /// returns network address as a host
    ///
    /// In newly created object, IP address is not changed. Prefix is set to /32 or /128 according to protocol.
    /// - 10.0.0.0/24 => 10.0.0.0/32
    /// - fe80::/64 => fe80::/128
    CIDRAddress host() const;

    /// returns network address of this host
    ///
    /// In newly created object, IP address is set to network according mask.
    /// - 10.0.0.1/24 => 10.0.0.0/24
    /// - fe80::ABCD/64 => fe80::/62
    CIDRAddress network() const;

    /// returns a broadcast address
    ///
    /// Method creates broadcast address
    ///     - 10.0.0.0/24 => 10.0.0.255
    /// In case of point-to-point or host it stil creates CIDRAddress object - copy of host address or highest address
    /// from point-to-point link.
    CIDRAddress broadcast() const;

    /// set address according parameter
    /// @return true on success (string is valid IP, we have enough memory, ...)
    bool set(const std::string& text);

    /// set address according parameter
    /// @return true when address is valid
    bool set(const CIDRAddress& from);
    bool set(const struct in_addr* address);
    bool set(const struct in6_addr* address);
    bool set(const struct sockaddr* address);

    /// converts address to string
    ///
    /// Returned string contains prefix, only if it isn't a host. Prefix 32 (or 128 for IPv6) is suppressed.
    /// @return string with IP address with or without prefix
    std::string toString() const;

    /// converts address to string
    ///
    /// Returned string is network address. You can enforce or suppress network mask using predefined constants.
    /// @param Formating option (CIDR_AUTO_PREFIX, CIDR_WITHOUT_PREFIX, CIDR_WITH_PREFIX)
    /// @return string with IP address with or without prefix
    std::string toString(CIDROptions option) const;

    /// compare two ip addresses
    ///
    /// Compares two ip addresses.
    /// - +1 if *this is bigger
    /// - -1 if *this is smaller
    /// - 0 if *this is equal
    ///
    /// Please consider those special cases:
    /// - when comparing IPv6 with IPv4, algorithm says that IPv6 is bigger (i. e. ::1 > 192.168.0.1).
    /// - when comparing invalid address, valid > invalid, invalid == invalid
    ///
    /// @see operator>
    /// @see operator<
    /// @see operator==;
    /// @return int
    int compare(const CIDRAddress& a2) const;
    ~CIDRAddress();

private:
    CIDR* _cidr; //!< private pointer to libcidr structure

    /// set cidr pointer to new value, free old one.
    /// @param newcidr (CIDR *) pointer to newly allocated CIDR structure;
    void setCidrPtr(CIDR* newcidr);
};

/// Overloaded << operator for output stream uses toString() method of CIDRAddress.
std::ostream& operator<<(std::ostream& os, const CIDRAddress& address);

/// @class CIDRList
///
/// Provide class for iterating over network segments
///
/// This class provides methods for iterating trough one or more network
/// segments. It supports excludes, so You can specify to walk trough
/// big segment and exclude some parts of it.
///
/// Exclude/include evaluation uses prefix for prioritization the same way
/// as routing does - longer prefix win. In case of equality exclude win.
///
///     CIDRList list;
///     list.add("1.2.3.0/24");
///     list.exclude("1.2.0.0/16"); //this will hide nothing (smaller prefix)
///     list.exclude("1.2.3.0/24"); //this will hide everything
class CIDRList
{
public:
    /// Creates new CIDRList object
    CIDRList();
    ~CIDRList();

    /// Add new network to includes
    /// @param string representing network address
    bool add(const std::string& net);

    /// Add new network to includes
    /// @param valid CIDRAddress object
    bool add(const CIDRAddress& net);

    /// Add new exclude network
    /// @param string representing network address
    bool exclude(const std::string& net);

    /// Add new exclude network
    /// @param valid CIDRAddress object
    bool exclude(const CIDRAddress& net);

    /// Set address to next in list.
    ///
    /// Method evaluates includes and excludes and then sets the address to new
    /// value. Network addresses and broadcasts are also filtered. If You realy
    /// need them in list, You have to add them specifically ( add("10.0.0.0/32") )
    ///
    /// If parameter CIDRAddress net is not valid ( net.valid() == false ) iteration
    /// starts and fisrt valid address is found.
    ///
    /// At the end of list, address parameter is set to be invalid again and false is returned.
    ///
    /// Simple example:
    ///
    ///     CIDRList list;
    ///     list.add("10.20.30.16/30");
    ///     list.add("10.21.31.0/24");
    ///     list.exclude("10.21.31.128/25");
    ///     CIDRAddress addr;
    ///     while( list.next(addr) ) {
    ///         std::cout << addr << "\n";
    ///     }
    ///
    /// in the future we need some clever mechanism for IPv6.
    ///
    /// @param CIDRAddress object.
    /// @return true, if next address has been found.
    bool next(CIDRAddress& address);

    /// find lowest address in list
    CIDRAddress firstAddress() const;

    /// Find highest address in list (broadcast excluded).
    CIDRAddress lastAddress() const;

    /// Find the network with largest prefix, where requested address is included.
    CIDRAddress bestNetworkFor(CIDRAddress& address) const;

    /// Find prefix len for inclusion (-1 = not in list).
    int bestNetworkPrefixFor(CIDRAddress& address) const;

    /// find the network with largest prefix, where requested address is excluded.
    CIDRAddress bestExcludeFor(CIDRAddress& address) const;

    /// Find prefix len for exclusion (-1 = not in list).
    int bestExcludePrefixFor(CIDRAddress& address) const;

    /// Simple evaluation if address is in included networks.
    bool includes(const CIDRAddress& address) const;

    /// Simple evaluation if address is in excluded networks.
    bool excludes(const CIDRAddress& address) const;

private:
    std::vector<CIDRAddress> _networks;         //!< list of includes
    std::vector<CIDRAddress> _excludedNetworks; //!< list of excludes
    CIDRAddress              _last;             //!< highest address from list

private:
    /// skipping gaps between networks
    void _skipToNextPool(CIDRAddress& address) const;

    /// skipping to the end of exclude or to beginning of next include
    void _skipToExcludeEnd(CIDRAddress& address) const;

    /// simple increment address of 1 eventually go at the beginning
    bool _nextSimple(CIDRAddress& address);
};


} // namespace shared
