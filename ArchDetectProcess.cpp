//g++ ArchDetectProcess.cpp -o ArchDetectProc / For linux
//x86_64-w64-mingw32-g++ ArchDetectProcess.cpp -o ArchDetectProc  /For windows
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <psapi.h>
#include <tchar.h>
#include <stdio.h>
#include <iomanip>


std::string GetSystemArchitectureString()
{
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);

    switch (si.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_INTEL:  return "x86";
        case PROCESSOR_ARCHITECTURE_AMD64:  return "x64";
        case PROCESSOR_ARCHITECTURE_ARM:    return "ARM";
        case PROCESSOR_ARCHITECTURE_ARM64:  return "ARM64";
        default:                            return "Unknown";
    }
}
typedef enum {
    BIN_UNKNOWN,
    BIN_PE_X86,
    BIN_PE_X64,
    BIN_ELF_X86,
    BIN_ELF_X64
} BinaryType;

BOOL GetProcessExePath(DWORD pid, TCHAR* exePath)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess)
        return FALSE;

    DWORD size = MAX_PATH;
    BOOL success = QueryFullProcessImageName(hProcess, 0, exePath, &size);

    CloseHandle(hProcess);
    return success;
}
BinaryType DetectFileType(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f) return BIN_UNKNOWN;

    unsigned char buffer[64];
    fread(buffer, 1, sizeof(buffer), f);

    // ----- Check ELF -----
    if (buffer[0] == 0x7F &&
        buffer[1] == 'E' &&
        buffer[2] == 'L' &&
        buffer[3] == 'F')
    {
        if (buffer[4] == 1)
            return BIN_ELF_X86;
        else if (buffer[4] == 2)
            return BIN_ELF_X64;
    }

    // ----- Check PE -----
    if (buffer[0] == 'M' && buffer[1] == 'Z')
    {
        uint32_t peOffset = *(uint32_t*)&buffer[0x3C];
        fseek(f, peOffset, SEEK_SET);

        char peSig[4];
        fread(peSig, 1, 4, f);

        if (memcmp(peSig, "PE\0\0", 4) == 0)
        {
            uint16_t machine;
            fread(&machine, 2, 1, f);

            if (machine == 0x014C)
                return BIN_PE_X86;
            else if (machine == 0x8664)
                return BIN_PE_X64;
        }
    }

    fclose(f);
    return BIN_UNKNOWN;
}

std::vector<std::string> PROCESS_ERROR;
HANDLE GetProcessRun(PROCESSENTRY32& pe)
{
    pe.dwSize = sizeof(pe);

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
    {
        std::cout << "\033[31m[DEBUG] >\033[0m Falha ao iniciar CreateToolhelp32Snapshot\n";
        return NULL;
    }
    else
    {
        if(!Process32First(snap, &pe))
        {
            std::cout << "\033[31m[DEBUG] >\033[0m Falha ao iniciar Process32First\n";
            return NULL;
        }
        else
        {
            while(true)
            {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, false, pe.th32ProcessID);
                
                if(!hProcess)
                {
                    PROCESS_ERROR.push_back(pe.szExeFile);
                    if(!Process32Next(snap, &pe))
                    {
                        std::cout << "\n\033[31m[DEBUG] >\033[0m Finalizado a leitura dos processos abertos no sistema...\n"; 
                        break;
                    }
                }
                else
                {
                    TCHAR exePath[MAX_PATH];
                    if (GetProcessExePath(pe.th32ProcessID, exePath))
                    {
                        BinaryType typeB = DetectFileType(exePath);
                        std::string arch = GetSystemArchitectureString();
                        std::string command = "title ArchDetect - Sistema de : " + arch;
                        system(command.c_str());

                        switch (typeB)
                        {
                            case BIN_PE_X86:
                                std::cout 
                                    << std::setw(10) << std::left << "\033[34mDEBUG >\033[0m"
                                    << std::setw(7)  << std::left << "PID" 
                                    << std::setw(7)  << std::left << pe.th32ProcessID
                                    << std::setw(7)  << std::left << "TID" 
                                    << std::setw(7)  << std::left << pe.cntThreads
                                    << std::setw(20) << std::left << "Arch : x86-64 (32-bit)" 
                                    << std::setw(20) << std::left << "Format : PE" 
                                    << std::setw(40) << std::left << pe.szExeFile
                                    << "\n";
                                break;

                            case BIN_PE_X64:
                                std::cout 
                                    << std::setw(10) << std::left << "\033[34mDEBUG >\033[0m"
                                    << std::setw(7)  << std::left << "PID" 
                                    << std::setw(7)  << std::left << pe.th32ProcessID
                                    << std::setw(7)  << std::left << "TID" 
                                    << std::setw(7)  << std::left << pe.cntThreads
                                    << std::setw(20) << std::left << "Arch : x86-64 (64-bit)" 
                                    << std::setw(20) << std::left << "Format : PE" 
                                    << std::setw(40) << std::left << pe.szExeFile
                                    << "\n";
                                break;

                            case BIN_ELF_X86:
                                std::cout 
                                    << std::setw(10) << std::left << "\033[34mDEBUG >\033[0m"
                                    << std::setw(7)  << std::left << "PID" 
                                    << std::setw(7)  << std::left << pe.th32ProcessID
                                    << std::setw(7)  << std::left << "TID" 
                                    << std::setw(7)  << std::left << pe.cntThreads
                                    << std::setw(20) << std::left << "Arch : x86-64 (32-bit)" 
                                    << std::setw(20) << std::left << "Format : ELF" 
                                    << std::setw(40) << std::left << pe.szExeFile
                                    << "\n";
                                break;

                            case BIN_ELF_X64:
                                std::cout 
                                    << std::setw(10) << std::left << "\033[34mDEBUG >\033[0m"
                                    << std::setw(7)  << std::left << "PID" 
                                    << std::setw(7)  << std::left << pe.th32ProcessID
                                    << std::setw(7)  << std::left << "TID" 
                                    << std::setw(7)  << std::left << pe.cntThreads
                                    << std::setw(20) << std::left << "Arch : x86-64 (64-bit)" 
                                    << std::setw(20) << std::left << "Format : ELF" 
                                    << std::setw(40) << std::left << pe.szExeFile
                                    << "\n";
                                break;

                            default:
                                printf("Format : Unknown\n");
                                printf("Arch   : Unknown\n");
                                break;
                        }
                    }
                    else
                    {
                        std::cout << L"\033[31m[DEBUG] >\033[0m Nao foi possivel obter o caminho do " << pe.szExeFile << "\n";
                        if(!Process32Next(snap, &pe))
                        {
                            std::cout << "\n\033[31m[DEBUG] >\033[0m Finalizado a leitura dos processos abertos no sistema...\n"; 
                            break;
                        }
                    } 
                  
                }
                if(!Process32Next(snap, &pe))
                {
                    std::cout << "\n\033[31m[DEBUG] >\033[0m Finalizado a leitura dos processos abertos no sistema...\n"; 
                    break;
                }
            }
        }
    }
    CloseHandle(snap);
    return NULL;
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
void enable_ansi_colors()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
void CheckAdmin()
{
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;

    // cria SID para grupo Administradores
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(
            &NtAuthority, 2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0,0,0,0,0,0,
            &adminGroup))
    {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    if (!isAdmin) {
        MessageBox(
            NULL,
            _T("Este programa precisa ser executado como administrador!\n\n"
               "Versao: 1.0.2Laste\n"
               "Desenvolvedor: Junior Andreli\n"
               "Email de suporte: juniorandreli39@gmail.com"),
            _T("Permissao Negada"),
            MB_ICONWARNING | MB_OK
        );
        ExitProcess(1);
    }
}
int main(int argc, const char* argv[])
{
    permanece_job_diretory();
    enable_ansi_colors();
    CheckAdmin();
    PROCESSENTRY32 pe;
    GetProcessRun(pe);
   
    while(true)
    {
        std::string response;
        std::cout << "\033[32m[DEBUG] >\033[0m A operao foi concluida, deseja escanear novamente todos os processos?(yes/no)> ";
        std::getline(std::cin, response);
        if(response.empty() || response != "yes")break;

        //Tentando novamente
        PROCESSENTRY32 pe;
        GetProcessRun(pe);

    }
    return 0;
}