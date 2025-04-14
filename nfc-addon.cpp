#include <napi.h>
#include <windows.h>
#include <winscard.h>
#include <sstream>
#include <vector>
#include <thread>
#include <atomic>

// ACR122U specific commands
const BYTE ACR122U_GET_FIRMWARE[] = {0xFF, 0x00, 0x48, 0x00, 0x00};
const BYTE ACR122U_GET_UID[] = {0xFF, 0xCA, 0x00, 0x00, 0x00};
const BYTE ACR122U_LOAD_AUTHENTICATION_KEYS[] = {0xFF, 0x82, 0x00, 0x00, 0x06, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const BYTE ACR122U_AUTHENTICATE[] = {0xFF, 0x86, 0x00, 0x00, 0x05, 0x01, 0x00, 0x00, 0x60, 0x00};

std::string ByteArrayToHex(BYTE *data, DWORD length)
{
    std::stringstream ss;
    for (DWORD i = 0; i < length; ++i)
    {
        ss << std::hex << std::uppercase;
        ss.width(2);
        ss.fill('0');
        ss << static_cast<int>(data[i]);
    }
    return ss.str();
}

std::string ParseNDEFText(const BYTE *data, DWORD length)
{
    if (length < 5)
        return "";

    // NDEF Text Record header (simplified)
    BYTE status = data[0];
    BYTE langLen = status & 0x3F;
    if (length < langLen + 1)
        return "";

    return std::string((char *)(data + 1 + langLen), length - 1 - langLen);
}

bool IsACR122U(const std::string &readerName)
{
    return readerName.find("ACR122U") != std::string::npos;
}

Napi::Value GetReaders(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    SCARDCONTEXT context;
    LONG lReturn = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context);

    if (lReturn != SCARD_S_SUCCESS)
    {
        std::string errorMsg = "Failed to establish context. Error code: " + std::to_string(lReturn);
        switch (lReturn)
        {
        case SCARD_E_NO_SERVICE:
            errorMsg += " (Smart Card service is not running)";
            break;
        case SCARD_E_NO_READERS_AVAILABLE:
            errorMsg += " (No readers available)";
            break;
        case SCARD_E_INVALID_PARAMETER:
            errorMsg += " (Invalid parameter)";
            break;
        case SCARD_E_INVALID_TARGET:
            errorMsg += " (Invalid target)";
            break;
        case SCARD_E_NO_MEMORY:
            errorMsg += " (Not enough memory)";
            break;
        default:
            errorMsg += " (Unknown error)";
            break;
        }
        Napi::Error::New(env, errorMsg).ThrowAsJavaScriptException();
        return env.Null();
    }

    LPTSTR mszReaders = NULL;
    DWORD dwReadersLen = SCARD_AUTOALLOCATE;
    lReturn = SCardListReaders(context, NULL, (LPTSTR)&mszReaders, &dwReadersLen);
    if (lReturn != SCARD_S_SUCCESS)
    {
        SCardReleaseContext(context);
        std::string errorMsg = "Failed to list readers. Error code: " + std::to_string(lReturn);
        Napi::Error::New(env, errorMsg).ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Array readers = Napi::Array::New(env);
    DWORD idx = 0;
    LPTSTR pReader = mszReaders;
    int i = 0;

    while (*pReader != '\0')
    {
        std::string readerName(pReader);
        if (IsACR122U(readerName))
        {
            // Prioritize ACR122U readers by putting them first
            readers.Set(static_cast<uint32_t>(0), Napi::String::New(env, readerName));
            i = 1;
        }
        else
        {
            readers.Set(static_cast<uint32_t>(i++), Napi::String::New(env, readerName));
        }
        pReader += strlen(pReader) + 1;
    }

    SCardFreeMemory(context, mszReaders);
    SCardReleaseContext(context);
    return readers;
}

Napi::Value GetReaderInfo(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "Reader name must be provided").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string readerName = info[0].As<Napi::String>().Utf8Value();
    SCARDCONTEXT context;
    LONG lReturn = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context);
    if (lReturn != SCARD_S_SUCCESS)
    {
        Napi::Error::New(env, "Failed to establish context").ThrowAsJavaScriptException();
        return env.Null();
    }

    SCARDHANDLE hCard;
    DWORD dwActiveProtocol;
    lReturn = SCardConnect(context, readerName.c_str(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
    if (lReturn != SCARD_S_SUCCESS)
    {
        SCardReleaseContext(context);
        Napi::Error::New(env, "Failed to connect to reader").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object result = Napi::Object::New(env);

    // Get firmware version for ACR122U
    if (IsACR122U(readerName))
    {
        BYTE pbRecvBuffer[256];
        DWORD dwRecvLength = sizeof(pbRecvBuffer);
        lReturn = SCardTransmit(hCard, SCARD_PCI_T1, ACR122U_GET_FIRMWARE, sizeof(ACR122U_GET_FIRMWARE), NULL, pbRecvBuffer, &dwRecvLength);
        if (lReturn == SCARD_S_SUCCESS && dwRecvLength >= 2)
        {
            std::string firmware = ByteArrayToHex(pbRecvBuffer, dwRecvLength);
            result.Set("firmware", firmware);
        }
    }

    SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    SCardReleaseContext(context);
    return result;
}

Napi::Value ReadTag(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "Reader name must be provided").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string readerName = info[0].As<Napi::String>().Utf8Value();
    SCARDCONTEXT context;
    LONG lReturn = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context);
    if (lReturn != SCARD_S_SUCCESS)
    {
        Napi::Error::New(env, "Failed to establish context").ThrowAsJavaScriptException();
        return env.Null();
    }

    SCARDHANDLE hCard;
    DWORD dwActiveProtocol;
    lReturn = SCardConnect(context, readerName.c_str(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
    if (lReturn != SCARD_S_SUCCESS)
    {
        SCardReleaseContext(context);
        Napi::Error::New(env, "Failed to connect to reader").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object result = Napi::Object::New(env);

    // For ACR122U, try to get UID first
    if (IsACR122U(readerName))
    {
        BYTE pbRecvBuffer[256];
        DWORD dwRecvLength = sizeof(pbRecvBuffer);
        lReturn = SCardTransmit(hCard, SCARD_PCI_T1, ACR122U_GET_UID, sizeof(ACR122U_GET_UID), NULL, pbRecvBuffer, &dwRecvLength);
        if (lReturn == SCARD_S_SUCCESS && dwRecvLength >= 2)
        {
            std::string uid = ByteArrayToHex(pbRecvBuffer, dwRecvLength);
            result.Set("uid", uid);
        }
    }

    // Try to read NDEF data
    BYTE pbRecvBuffer[256];
    DWORD dwRecvLength = sizeof(pbRecvBuffer);
    BYTE pbSendBuffer[] = {0xFF, 0xB0, 0x00, 0x00, 0x10};
    lReturn = SCardTransmit(hCard, SCARD_PCI_T1, pbSendBuffer, sizeof(pbSendBuffer), NULL, pbRecvBuffer, &dwRecvLength);
    if (lReturn == SCARD_S_SUCCESS && dwRecvLength >= 2)
    {
        std::string data = ByteArrayToHex(pbRecvBuffer, dwRecvLength);
        result.Set("data", data);
        result.Set("text", ParseNDEFText(pbRecvBuffer, dwRecvLength));
    }

    SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    SCardReleaseContext(context);
    return result;
}

Napi::Value WriteTag(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString())
    {
        Napi::TypeError::New(env, "Reader name and data must be provided").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string readerName = info[0].As<Napi::String>().Utf8Value();
    std::string data = info[1].As<Napi::String>().Utf8Value();
    SCARDCONTEXT context;
    LONG lReturn = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context);
    if (lReturn != SCARD_S_SUCCESS)
    {
        Napi::Error::New(env, "Failed to establish context").ThrowAsJavaScriptException();
        return env.Null();
    }

    SCARDHANDLE hCard;
    DWORD dwActiveProtocol;
    lReturn = SCardConnect(context, readerName.c_str(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
    if (lReturn != SCARD_S_SUCCESS)
    {
        SCardReleaseContext(context);
        Napi::Error::New(env, "Failed to connect to reader").ThrowAsJavaScriptException();
        return env.Null();
    }

    // For ACR122U, try to authenticate first
    if (IsACR122U(readerName))
    {
        lReturn = SCardTransmit(hCard, SCARD_PCI_T1, ACR122U_LOAD_AUTHENTICATION_KEYS, sizeof(ACR122U_LOAD_AUTHENTICATION_KEYS), NULL, NULL, NULL);
        if (lReturn == SCARD_S_SUCCESS)
        {
            lReturn = SCardTransmit(hCard, SCARD_PCI_T1, ACR122U_AUTHENTICATE, sizeof(ACR122U_AUTHENTICATE), NULL, NULL, NULL);
        }
    }

    BYTE pbRecvBuffer[256];
    DWORD dwRecvLength = sizeof(pbRecvBuffer);
    BYTE pbSendBuffer[] = {0xFF, 0xD6, 0x00, 0x00, 0x10};
    lReturn = SCardTransmit(hCard, SCARD_PCI_T1, pbSendBuffer, sizeof(pbSendBuffer), NULL, pbRecvBuffer, &dwRecvLength);
    if (lReturn != SCARD_S_SUCCESS)
    {
        SCardDisconnect(hCard, SCARD_LEAVE_CARD);
        SCardReleaseContext(context);
        Napi::Error::New(env, "Failed to write data").ThrowAsJavaScriptException();
        return env.Null();
    }

    SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    SCardReleaseContext(context);
    return Napi::Boolean::New(env, true);
}

Napi::Value SendApdu(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString())
    {
        Napi::TypeError::New(env, "Reader name and APDU command must be provided").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string readerName = info[0].As<Napi::String>().Utf8Value();
    std::string apdu = info[1].As<Napi::String>().Utf8Value();
    SCARDCONTEXT context;
    LONG lReturn = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context);
    if (lReturn != SCARD_S_SUCCESS)
    {
        Napi::Error::New(env, "Failed to establish context").ThrowAsJavaScriptException();
        return env.Null();
    }

    SCARDHANDLE hCard;
    DWORD dwActiveProtocol;
    lReturn = SCardConnect(context, readerName.c_str(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
    if (lReturn != SCARD_S_SUCCESS)
    {
        SCardReleaseContext(context);
        Napi::Error::New(env, "Failed to connect to reader").ThrowAsJavaScriptException();
        return env.Null();
    }

    BYTE pbRecvBuffer[256];
    DWORD dwRecvLength = sizeof(pbRecvBuffer);
    BYTE pbSendBuffer[256];
    DWORD dwSendLength = 0;

    // Convert hex string to byte array
    for (size_t i = 0; i < apdu.length(); i += 2)
    {
        std::string byteString = apdu.substr(i, 2);
        pbSendBuffer[dwSendLength++] = (BYTE)strtol(byteString.c_str(), NULL, 16);
    }

    lReturn = SCardTransmit(hCard, SCARD_PCI_T1, pbSendBuffer, dwSendLength, NULL, pbRecvBuffer, &dwRecvLength);
    if (lReturn != SCARD_S_SUCCESS)
    {
        SCardDisconnect(hCard, SCARD_LEAVE_CARD);
        SCardReleaseContext(context);
        Napi::Error::New(env, "Failed to send APDU command").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string response = ByteArrayToHex(pbRecvBuffer, dwRecvLength);
    SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    SCardReleaseContext(context);
    return Napi::String::New(env, response);
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports.Set("getReaders", Napi::Function::New(env, GetReaders));
    exports.Set("getReaderInfo", Napi::Function::New(env, GetReaderInfo));
    exports.Set("readTag", Napi::Function::New(env, ReadTag));
    exports.Set("writeTag", Napi::Function::New(env, WriteTag));
    exports.Set("sendApdu", Napi::Function::New(env, SendApdu));
    return exports;
}

NODE_API_MODULE(nfcaddon, Init)
