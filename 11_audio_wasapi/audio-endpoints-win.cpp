#include "audio-io.h"

#ifdef _WIN32

// https://docs.microsoft.com/en-us/windows/win32/coreaudio/device-properties
int enum_audio_endpoints()
{
    IMMDeviceEnumerator *enumerator = nullptr;
    IMMDeviceCollection *collection = nullptr;
    IMMDevice *endpoint = nullptr;
    LPWSTR id = nullptr;
    IPropertyStore *props = nullptr;
    UINT count = 0;

    RETURN_ON_ERROR(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    defer(CoUninitialize());

    RETURN_ON_ERROR(CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            nullptr,
            CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            (void **)&enumerator
    ));
    defer(SAFE_RELEASE(enumerator));


    RETURN_ON_ERROR(enumerator->EnumAudioEndpoints(
            eRender,
            DEVICE_STATE_ACTIVE,
            &collection
    ));
    defer(SAFE_RELEASE(collection));

    RETURN_ON_ERROR(collection->GetCount(&count));
    for (ULONG i = 0; i < count; i++) {
        // Get pointer to endpoint number i.
        RETURN_ON_ERROR(collection->Item(i, &endpoint));
        defer(SAFE_RELEASE(endpoint));

        // Get the endpoint ID string.
        RETURN_ON_ERROR(endpoint->GetId(&id));
        defer(CoTaskMemFree(id); id = nullptr);

        RETURN_ON_ERROR(endpoint->OpenPropertyStore(STGM_READ, &props));
        defer(SAFE_RELEASE(props));

        PROPVARIANT varName;
        // Initialize container for property value.
        PropVariantInit(&varName);
        defer(PropVariantClear(&varName));

        // Get the endpoint's friendly-name property.
        RETURN_ON_ERROR(props->GetValue(PKEY_Device_FriendlyName, &varName));

        // Print endpoint friendly name and endpoint ID.
        printf("Endpoint %lu: \"%S\" (%S)\n", i, varName.pwszVal, id);
    }

    return 0;
}

int default_audio_endpoint()
{
    IMMDeviceEnumerator *enumerator = nullptr;
    IMMDeviceCollection *collection = nullptr;
    IMMDevice *endpoint = nullptr;
    LPWSTR id = nullptr;
    IPropertyStore *props = nullptr;

    RETURN_ON_ERROR(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    defer(CoUninitialize());

    RETURN_ON_ERROR(CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            nullptr,
            CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            (void **)&enumerator
    ));
    defer(SAFE_RELEASE(enumerator));

    RETURN_ON_ERROR(enumerator->EnumAudioEndpoints(
            eRender,
            DEVICE_STATE_ACTIVE,
            &collection
    ));
    defer(SAFE_RELEASE(collection));

    RETURN_ON_ERROR(enumerator->GetDefaultAudioEndpoint(
            eRender,
            eConsole,
            &endpoint
    ));
    defer(SAFE_RELEASE(endpoint));

    RETURN_ON_ERROR(endpoint->GetId(&id));
    defer(CoTaskMemFree(id); id = nullptr);

    RETURN_ON_ERROR(endpoint->OpenPropertyStore(STGM_READ, &props));
    defer(SAFE_RELEASE(props));

    PROPVARIANT varName;
    // Initialize container for property value.
    PropVariantInit(&varName);
    defer(PropVariantClear(&varName));

    // Get the endpoint's friendly-name property.
    RETURN_ON_ERROR(props->GetValue(PKEY_Device_FriendlyName, &varName));

    // Print endpoint friendly name and endpoint ID.
    printf("Default Endpoint: \"%S\" (%S)\n", varName.pwszVal, id);
    return 0;
}

#endif //_WIN32