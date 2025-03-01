// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <unordered_map>
#include <utility>
#include <boost/serialization/unordered_map.hpp>
#include "core/hle/result.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::SOC {

/// Holds information about a particular socket
struct SocketHolder {
#ifdef _WIN32
    using SOCKET = unsigned long long;
    SOCKET socket_fd; ///< The socket descriptor
#else
    int socket_fd; ///< The socket descriptor
#endif // _WIN32

    bool blocking = true; ///< Whether the socket is blocking or not.

private:
    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& socket_fd;
        ar& blocking;
    }
    friend class boost::serialization::access;
};

class SOC_U final : public ServiceFramework<SOC_U> {
public:
    SOC_U();
    ~SOC_U();

    struct InterfaceInfo {
        u32 address;
        u32 netmask;
        u32 broadcast;
    };

    // Gets the interface info that is able to reach the internet.
    std::optional<InterfaceInfo> GetDefaultInterfaceInfo();

private:
    static constexpr ResultCode ERR_INVALID_HANDLE =
        ResultCode(ErrorDescription::InvalidHandle, ErrorModule::SOC, ErrorSummary::InvalidArgument,
                   ErrorLevel::Permanent);
    static constexpr u32 SOC_ERR_INAVLID_ENUM_VALUE = 0xFFFF8025;

    static constexpr u32 SOC_SOL_IP = 0x0000;
    static constexpr u32 SOC_SOL_TCP = 0x0006;
    static constexpr u32 SOC_SOL_CONFIG = 0xFFFE;
    static constexpr u32 SOC_SOL_SOCKET = 0xFFFF;

    static const std::unordered_map<u64, std::pair<int, int>> sockopt_map;
    static std::pair<int, int> TranslateSockOpt(int level, int opt);
    bool GetSocketBlocking(const SocketHolder& socket_holder);
    u32 SetSocketBlocking(SocketHolder& socket_holder, bool blocking);

    // From
    // https://github.com/devkitPro/libctru/blob/1de86ea38aec419744149daf692556e187d4678a/libctru/include/3ds/services/soc.h#L15
    enum class NetworkOpt {
        NETOPT_MAC_ADDRESS = 0x1004,     ///< The mac address of the interface
        NETOPT_ARP_TABLE = 0x3002,       ///< The ARP table
        NETOPT_IP_INFO = 0x4003,         ///< The current IP setup
        NETOPT_IP_MTU = 0x4004,          ///< The value of the IP MTU
        NETOPT_ROUTING_TABLE = 0x4006,   ///< The routing table
        NETOPT_UDP_NUMBER = 0x8002,      ///< The number of sockets in the UDP table
        NETOPT_UDP_TABLE = 0x8003,       ///< The table of opened UDP sockets
        NETOPT_TCP_NUMBER = 0x9002,      ///< The number of sockets in the TCP table
        NETOPT_TCP_TABLE = 0x9003,       ///< The table of opened TCP sockets
        NETOPT_DNS_TABLE = 0xB003,       ///< The table of the DNS servers
        NETOPT_DHCP_LEASE_TIME = 0xC001, ///< The DHCP lease time remaining, in seconds
    };

    struct HostByNameData {
        static const u32 max_entries = 24;

        u16_le addr_type;
        u16_le addr_len;
        u16_le addr_count;
        u16_le alias_count;
        std::array<char, 256> h_name;
        std::array<std::array<char, 256>, max_entries> aliases;
        std::array<std::array<u8, 16>, max_entries> addresses;
    };
    static_assert(sizeof(HostByNameData) == 0x1A88, "Invalid HostByNameData size");

    void Socket(Kernel::HLERequestContext& ctx);
    void Bind(Kernel::HLERequestContext& ctx);
    void Fcntl(Kernel::HLERequestContext& ctx);
    void Listen(Kernel::HLERequestContext& ctx);
    void Accept(Kernel::HLERequestContext& ctx);
    void GetHostId(Kernel::HLERequestContext& ctx);
    void Close(Kernel::HLERequestContext& ctx);
    void SendToOther(Kernel::HLERequestContext& ctx);
    void SendTo(Kernel::HLERequestContext& ctx);
    void RecvFromOther(Kernel::HLERequestContext& ctx);
    void RecvFrom(Kernel::HLERequestContext& ctx);
    void Poll(Kernel::HLERequestContext& ctx);
    void GetSockName(Kernel::HLERequestContext& ctx);
    void Shutdown(Kernel::HLERequestContext& ctx);
    void GetHostByName(Kernel::HLERequestContext& ctx);
    void GetPeerName(Kernel::HLERequestContext& ctx);
    void Connect(Kernel::HLERequestContext& ctx);
    void InitializeSockets(Kernel::HLERequestContext& ctx);
    void ShutdownSockets(Kernel::HLERequestContext& ctx);
    void GetSockOpt(Kernel::HLERequestContext& ctx);
    void SetSockOpt(Kernel::HLERequestContext& ctx);
    void GetNetworkOpt(Kernel::HLERequestContext& ctx);

    // Some platforms seem to have GetAddrInfo and GetNameInfo defined as macros,
    // so we have to use a different name here.
    void GetAddrInfoImpl(Kernel::HLERequestContext& ctx);
    void GetNameInfoImpl(Kernel::HLERequestContext& ctx);

    // Socked ids
    u32 next_socket_id = 3;
    u32 GetNextSocketID() {
        return next_socket_id++;
    }

    // System timer adjust
    std::chrono::time_point<std::chrono::steady_clock> adjust_value_last;
    void PreTimerAdjust();
    void PostTimerAdjust(Kernel::HLERequestContext& ctx, const std::string& caller_method);

    /// Close all open sockets
    void CleanupSockets();

    /// Holds info about the currently open sockets
    friend struct CTRPollFD;
    std::unordered_map<u32, SocketHolder> open_sockets;

    /// Cache interface info for the current session
    /// These two fields are not saved to savestates on purpose
    /// as network interfaces may change and it's better to.
    /// obtain them again between play sessions.
    bool interface_info_cached = false;
    InterfaceInfo interface_info;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& boost::serialization::base_object<Kernel::SessionRequestHandler>(*this);
        ar& open_sockets;
    }
    friend class boost::serialization::access;
};

std::shared_ptr<SOC_U> GetService(Core::System& system);

void InstallInterfaces(Core::System& system);

} // namespace Service::SOC

BOOST_CLASS_EXPORT_KEY(Service::SOC::SOC_U)
