#include "broker_client.h"
#include <iostream>

namespace Sar {

BrokerClient::BrokerClient() {}

BrokerClient::~BrokerClient()
{
    disconnect();
}

bool BrokerClient::connect()
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

bool BrokerClient::registerClient(const BrokerRegisterRequest &req, BrokerRegisterResponse &resp)
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

void BrokerClient::disconnect()
{
    if (_pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(_pipe);
        _pipe = INVALID_HANDLE_VALUE;
    }
}

} // namespace Sar
