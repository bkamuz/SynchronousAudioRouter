
#pragma once

#include <windows.h>
#include <string>
#include "broker_protocol.h"

namespace Sar {

class BrokerClient {
public:
    BrokerClient();
    ~BrokerClient();

    bool connect();
    bool registerClient(const BrokerRegisterRequest &req, BrokerRegisterResponse &resp);
    void disconnect();

private:
    HANDLE _pipe = INVALID_HANDLE_VALUE;
};

} // namespace Sar

// Inline implementations to avoid separate .cpp in this prototype.
namespace Sar {

inline BrokerClient::BrokerClient() {}

inline BrokerClient::~BrokerClient() { disconnect(); }

inline bool BrokerClient::connect()
{
    if (_pipe != INVALID_HANDLE_VALUE) return true;

    _pipe = CreateFileW(
        kSarAsioBrokerPipeName,
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, 0, nullptr);

    if (_pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    return true;
}

inline bool BrokerClient::registerClient(const BrokerRegisterRequest &req, BrokerRegisterResponse &resp)
{
    if (!connect()) return false;

    DWORD written = 0;
    if (!WriteFile(_pipe, &req, sizeof(req), &written, nullptr) || written != sizeof(req)) {
        return false;
    }

    DWORD read = 0;
    if (!ReadFile(_pipe, &resp, sizeof(resp), &read, nullptr) || read != sizeof(resp)) {
        return false;
    }

    return true;
}

inline void BrokerClient::disconnect()
{
    if (_pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(_pipe);
        _pipe = INVALID_HANDLE_VALUE;
    }
}

} // namespace Sar
