//x86_64-w64-mingw32-g++ VadoraPMemoryLastVersion.cpp -o VadoraPMemoryLastVersion.exe
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <windows.h>
#include <tlhelp32.h>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <mutex>

std::atomic<bool> ProgramaStatus(true);

void printMemoryScanResume()
{
    std::cout << "\n\n\033[36m[RESUMO]\033[0m Criterios de mapeamento analisados\n"
              << "--------------------------------------------------------------------------------------------------------\n"
              << "  Estado da memoria : MEM_COMMIT\n"
              << "  Protecoes         :"<< " | NOACCESS | PAGE_GUARD |- READ / WRITE / EXEC\n\n";
}

void printMemoryRegion(const MEMORY_BASIC_INFORMATION& mbi, const PROCESSENTRY32& pe)
{
    std::mutex LOCK;
    
    static bool headerPrinted = false;
    std::string type;

    if (mbi.State == MEM_COMMIT) type += "MEM_COMMIT ";
    if (mbi.Protect & PAGE_NOACCESS) type += "NOACCESS ";
    if (mbi.Protect & PAGE_GUARD) type += "PAGE_GUARD ";
    if (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE |
        PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))
        type += "READ/WRITE/EXEC ";

    if (!type.empty())
    {
        if (!headerPrinted)
        {
            std::cout
                << std::left
                << std::setw(45) << "TYPE / STATE / PROTECT"
                << std::right
                << std::setw(8)  << "PID"
                << std::setw(8)  << "TIDs"
                << " "
                << std::setw(20) << "ADDRESS"
                << std::setw(20) << "SIZE"
                << " PERMI"
                << std::endl;
            std::lock_guard<std::mutex> lock(LOCK);
            std::cout
                << std::left
                << std::setw(45) << "==========================================="
                << std::right
                << std::setw(8)  << "======="
                << std::setw(8)  << "======="
                << " "
                << std::setw(20) << "===================="
                << std::setw(20) << "===================="
                << " =========="
                << std::endl;

            headerPrinted = true;
        }

        std::cout
            << std::left
            << std::setw(45) << type
            << std::right
            << std::setw(8)  << pe.th32ProcessID
            << std::setw(8)  << pe.cntThreads
            << " "
            << std::setw(20) << std::hex << mbi.BaseAddress
            << std::setw(20) << std::dec << mbi.RegionSize
            << " 0x" << std::hex << std::setw(8) << std::setfill('0') << mbi.Protect
            << std::dec << std::setfill(' ')
            << std::endl;
    }
}

HANDLE ProcessHandleByName(PROCESSENTRY32 &pe, std::string &name)
{
    pe.dwSize = sizeof(pe);
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(snap == INVALID_HANDLE_VALUE || !snap)
    {
        std::cout << "\033[31mDEBUG > falhou CreateToolhelp (CODE: " << GetLastError() << ")\033[0m\n";
        return NULL; 
    }
    else
    {
        if(!Process32First(snap, &pe))
        {
            std::cout << "\033[31mDEBUG > falha inicializar processos capturados (CODE: " << GetLastError() << ")\033[0m\n";
            return NULL;
        }
        else
        {
            while(true)
            {
                if(strcmp(pe.szExeFile, name.c_str()) == 0)
                {
                    std::cout << "\033[31mDEBUG >\033[0m Processo encontrado para " << pe.szExeFile << ", abrindo processo ...\n";
                   HANDLE hProcess = OpenProcess(
                    PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_READ | PROCESS_VM_WRITE ,
                    false,
                    pe.th32ProcessID
                   );
                    if(hProcess == INVALID_HANDLE_VALUE || !hProcess)
                    {
                        std::cout << "\033[31mDEBUG > Falhou abertura do processo ("<< pe.szExeFile << "),ou eleve sua permisao pra admistrador (CODE: " << GetLastError() << ")\033[0m\n";
                        break;
                    }
                    else
                    {
                        ProgramaStatus = false;
                        CloseHandle(snap);
                        return hProcess;
                    }
                   
                }
                if(!Process32Next(snap, &pe))
                {
                    if(ProgramaStatus == true)
                    {
                        std::cout << "\033[31mDEBUG >\033[0m falha ao encontrar processo " << name << " :: Verifique gestor de tarefas para ver se o processo esta activo\n";
                        ProgramaStatus = true;
                    }
                    break;
                }
            }
        }
    }

    CloseHandle(snap);
    return NULL;
}

int main(int argc, const char* argv[])
{

    std::string STR_NAME_PROCESS;

    for(int i = 1; i < argc; i++)
    {
        std::string STR_VALUE = argv[i];
        if(STR_VALUE == "--pname" & i + 1 < argc)
        {
            STR_NAME_PROCESS = argv[i + i];
            i++;
        }
        else
        {
            continue;
        }
    }
    //std::cout <<"Hello\n";
    if(STR_NAME_PROCESS.empty())
    {
        std::cout << "Use > " << argv[0] << " --pname <ProcessName>\nVersao: AN-0.0.1Latest | 13-02-2026\n";
        return 1;
    }
    else
    {
        system("cls");
        system(("title DEBUG Buscando por processo " + STR_NAME_PROCESS).c_str());

        MEMORY_BASIC_INFORMATION mbi;
        PROCESSENTRY32 pe;

        HANDLE hProcess = ProcessHandleByName(pe, STR_NAME_PROCESS);
        if(!hProcess)
        {
            return 1;
        }
        else
        {
            std::cout << "\033[34m[INFO]\033[0m "<< "Iniciando leitura de regioes de memoria do processo > " << " PID: " << pe.th32ProcessID << " Nome: " << pe.szExeFile << "\n\n";
            LPVOID address = 0;
            while(VirtualQueryEx(hProcess, address, &mbi, sizeof(mbi)))
            {
                
                address = (LPVOID)((char*)mbi.BaseAddress + mbi.RegionSize);
                printMemoryRegion(mbi, pe);
            }
            printMemoryScanResume();
            
        }

    }

    return 0;
}


