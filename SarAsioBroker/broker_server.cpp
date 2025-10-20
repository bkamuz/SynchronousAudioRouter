#include <windows.h>
#include <stdio.h>
#include <string>
#include "..\\SarAsio\\broker_protocol.h"

static std::wstring MakeMappingName(DWORD pid, uint32_t index)
{
    wchar_t buf[128];
    swprintf_s(buf, L"Global\\SarAsioBuf_%u_%u", pid, index);
    return std::wstring(buf);
}

int wmain()
{
    printf("SarAsioBroker starting...\n");

    while (true) {
        HANDLE pipe = CreateNamedPipe(
            kSarAsioBrokerPipeName,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            4096, 4096,
            0, nullptr);

        if (pipe == INVALID_HANDLE_VALUE) {
            printf("CreateNamedPipe failed: %u\n", GetLastError());
            return 1;
        }

        printf("Waiting for client...\n");
        BOOL connected = ConnectNamedPipe(pipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (!connected) {
            CloseHandle(pipe);
            continue;
        }

        printf("Client connected\n");

        // Read request
        BrokerRegisterRequest req = {};
        DWORD read = 0;
        if (!ReadFile(pipe, &req, sizeof(req), &read, nullptr) || read != sizeof(req)) {
            printf("Failed to read request: %u\n", GetLastError());
            DisconnectNamedPipe(pipe);
            CloseHandle(pipe);
            continue;
        }

        if (req.msgType != BROKER_MSG_REGISTER) {
            printf("Unknown message type: %u\n", req.msgType);
            DisconnectNamedPipe(pipe);
            CloseHandle(pipe);
            continue;
        }

        // Create shared memory for this client
        std::wstring mappingName = MakeMappingName(req.pid, req.endpointIndex);
        uint32_t mappingSize = 65536; // simple fixed size for prototype

        HANDLE mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
            0, mappingSize, mappingName.c_str());

        if (!mapping) {
            printf("CreateFileMapping failed: %u\n", GetLastError());
        }

        BrokerRegisterResponse resp = {};
        resp.msgType = BROKER_MSG_REGISTER_RESPONSE;
        resp.status = mapping ? 0 : 1;
        wcscpy_s(resp.sectionName, mappingName.c_str());
        resp.mappingSize = mappingSize;
        resp.controlOffset = 0;

        DWORD written = 0;
        if (!WriteFile(pipe, &resp, sizeof(resp), &written, nullptr) || written != sizeof(resp)) {
            printf("Failed to write response: %u\n", GetLastError());
        }

        // Keep the pipe open until client disconnects; for prototype just wait and then close
        printf("Broker: registration completed, waiting for client disconnect\n");
        // Wait for client to disconnect
        char buffer[128];
        while (true) {
            DWORD r = 0;
            BOOL ok = ReadFile(pipe, buffer, sizeof(buffer), &r, nullptr);
            if (!ok || r == 0) break;
            // ignore
        }

        printf("Client disconnected, cleaning up\n");

        if (mapping) CloseHandle(mapping);
        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }

    return 0;
}
