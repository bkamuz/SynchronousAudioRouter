#pragma once

#include <windows.h>
#include <cstdint>

// Simple fixed-format protocol between SarAsioBroker and clients.
// All fields are little-endian native-sized and packed.

enum BrokerMessageType : uint32_t {
    BROKER_MSG_REGISTER = 1,
    BROKER_MSG_REGISTER_RESPONSE = 2,
    BROKER_MSG_UNREGISTER = 3,
};

// Client -> Broker
#pragma pack(push, 1)
struct BrokerRegisterRequest {
    uint32_t msgType; // BROKER_MSG_REGISTER
    uint32_t pid;
    uint32_t endpointIndex; // optional: which SAR endpoint index this client targets
    uint32_t channels;
    uint32_t frameSize; // frames per period
    uint32_t sampleSize; // bytes per sample
    uint32_t sampleRate;
    uint32_t direction; // 0 = playback (client -> SAR), 1 = recording (SAR -> client)
};

// Broker -> Client
struct BrokerRegisterResponse {
    uint32_t msgType; // BROKER_MSG_REGISTER_RESPONSE
    uint32_t status; // 0 success, non-zero error
    wchar_t sectionName[64]; // name of file-mapping (wide string)
    uint32_t mappingSize; // total mapping size in bytes
    uint32_t controlOffset; // offset in mapping where control block lives
};
#pragma pack(pop)

static const wchar_t kSarAsioBrokerPipeName[] = L"\\.\\pipe\\SarAsioBroker";
