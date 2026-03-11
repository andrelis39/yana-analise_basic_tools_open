//x86_64-w64-mingw32-g++ SeEProcess.cpp -o SeEProcess.exe
#include <string>
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <atomic>

std::atomic<bool> STATUS_PROCESS(true);

HANDLE hProcessByName(std::string &name, PROCESSENTRY32 &pe)
{
    pe.dwSize = sizeof(pe);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(!snapshot || snapshot == INVALID_HANDLE_VALUE)
    {
        std::cout << "\033[31mDEBUG >\033[0m falha CreateToolhelp (CODE: " << GetLastError() << ").\n";
        return NULL;
    }
    else
    {
        if(!Process32First(snapshot, &pe))
        {
            std::cout << "\033[31mDEBUG >\033[0m falha Process32 (CODE: " << GetLastError() << ").\n";
            return NULL;
        }
        else
        {
            while(true)
            {
                if(strcmp(pe.szExeFile, name.c_str()) == 0)
                {
                    STATUS_PROCESS = false;
                    std::cout << "\033[32mDEBUG >\033[0m Processo encontrado :: " << pe.szExeFile << "\n";
                    return snapshot; 
                }
                if(!Process32Next(snapshot, &pe))
                {
                    if(STATUS_PROCESS == true)
                    {
                        STATUS_PROCESS = true;
                        std::cout << "\033[31mDEBUG >\033[0m Processo nao encontrado :: " << pe.szExeFile << "\n";
                        break;
                    }
                }
            }
        }
    }

    CloseHandle(snapshot);
    return NULL;
}

int main(int argc, const char* argv[])
{
    system("cls");
    system("title Buscador de processos");

    std::string NPROCESS;
    for(int i = 1; i < argc; i++)
    {
        std::string VALUE = argv[i];
        if(VALUE == "--pname" && i + 1 < argc)
        {
            NPROCESS = argv[i + 1];
            i++;
        }
        continue;   
    }
    if(NPROCESS.empty())
    {
        std::cout << "\033[31mUse >\033[0m " << argv[0] << " --pname <NameProcess>\n";
        return 1;
    }
    PROCESSENTRY32 pe;
    

    HANDLE hProcess = hProcessByName(NPROCESS, pe);
    if(!hProcess) return 1;
    std::cout << "................................................................................................\nInformation process\n";
    std::cout << "\033[32mDEBUG >\033[0m PID:: " << pe.th32ProcessID << " TIDs:: " << pe.cntThreads << " NAME:: " << pe.szExeFile << "\n";

    return 0;
}