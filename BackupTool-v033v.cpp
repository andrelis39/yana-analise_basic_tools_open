//x86_64-w64-mingw32-g++ -std=c++17 BackupTool-v033v.cpp -o BackupTool-v033v.exe
#include <iostream>
#include <filesystem>
#include <windows.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <thread>
#include <iomanip>


namespace fs = std::filesystem;

void GetPrintDebug(std::string codeErr, std::string messageErr, std::string getNameFile)
{
    std::cout << "\033[31m[DEBUG] >\033[0m Code " << codeErr << " > " << messageErr << getNameFile <<"\n"; 
}
std::string GetCurrentDate()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm localTime;

    #ifdef _WIN32
        localtime_s(&localTime, &t);
    #else
        localtime_r(&t, &localTime);
    #endif

        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(2) << localTime.tm_mday << "-"
            << std::setw(2) << (localTime.tm_mon + 1) << "-"
            << (localTime.tm_year + 1900);

        return oss.str();
}
 void StarBackup(fs::path& origem, fs::path& destino)

    {
        SetConsoleTitleA(("Copiando arquivos para "+ destino.string()).c_str());
        std::cout <<"\n\033[31m[DEBUG] >\033[0m Copiando arquivos > " << origem.string() << " > " << destino.string() << "\n\n";
        try
        {
            if (!fs::exists(origem) || !fs::is_directory(origem))
            {
                
                DWORD code  = GetLastError();
                std::string message = "Pasta de origem invalida :: ";
                GetPrintDebug(std::to_string(code), message, origem.string());
                return;
            }

            if (!fs::exists(destino))
                fs::create_directories(destino);

            int num = 0;
            for (const auto& entry : fs::recursive_directory_iterator(origem))
            {
                const auto& caminhoOrigem = entry.path();
                auto caminhoDestino = destino / fs::relative(caminhoOrigem, origem);

                if (fs::is_directory(caminhoOrigem))
                {
                    fs::create_directories(caminhoDestino);
                }
                else if (fs::is_regular_file(caminhoOrigem))
                {
                    //Verificar a extensao do arquivo
                    std::string ext = caminhoOrigem.extension().string();

                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if(!(ext == ".mp4" || ext == ".mkvi" || ext == ".avi" || ext == ".mp3"))
                    {
                        fs::copy_file(caminhoOrigem, caminhoDestino,fs::copy_options::overwrite_existing);

                        std::cout << "\033[34m" << std::setw(10) << std::left << "[+]\033[0m" <<"\033[33m" << std::setw(15) << std::left << num <<"\033[0m"
                        << std::setw(30) << caminhoOrigem.string() <<"\n"; 
                        num ++;
                    }
                    continue;
                    
                }
            }
        }
        catch (const fs::filesystem_error& e)
        {
            DWORD code  = GetLastError();
            std::string message = "Falha ao copiar para :: ";
            GetPrintDebug(std::to_string(code), message, origem.string());
            return;
        }
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
int main(int argc, const char* argv[])
{
    
    permanece_job_diretory();
    enable_ansi_colors();
    if (argc < 3)
    {
        std::cout << "Use > " << argv[0] << " <Driver> <Token>\n";
        return 1;
    }

    // Obter username
    const char* user = std::getenv("USERNAME");
    if (!user) {
        std::cout << "[ERRO] Nao foi possivel obter USERNAME\n";
        return 1;
    }


    int len = MultiByteToWideChar(CP_UTF8, 0, user, -1, NULL, 0);
    std::wstring wuser(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, user, -1, &wuser[0], len);
    wuser.resize(wcslen(wuser.c_str()));

    //Assinatura
    fs::path tokenOrigem = fs::path(std::string(argv[2]));
    fs::path tokenName = fs::path(std::string(argv[2])).filename();
    fs::path tokenDestino = fs::path(std::string(argv[1]) + ":\\.base\\sys") / tokenName;
    
    //Origem vs destino de arquivo backup
    fs::path origem = fs::path("C:\\Users") / user / "Desktop\\Tool\\Export";
    fs::path destino = fs::path(std::string(argv[1]) + ":\\Export");
    fs::path origem2 = fs::path("C:\\Users") / user / "Desktop\\Tool\\PRIVATE";
    fs::path destino2 = fs::path(std::string(argv[1]) + ":\\PRIVATE");

    if(!fs::is_regular_file(tokenDestino))
    {
        //Copiando arquivos origem vs destino
        StarBackup(origem, destino);
        StarBackup(origem2, destino2);
        std::cout << "\033[34m[SUCESSO] >\033[0m Backup realizado com sucesso!\n";
        
        //Enviando assinatura
        fs::copy_file(tokenOrigem, tokenDestino,fs::copy_options::overwrite_existing);
        std::cout << "\033[34mDEBUG >\033[0m Assinatura token enviado com sucesso > \n";
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    else
    {
        std::cout << "\033[31mDEBUG >\033[0m Assinatura token encontrado > backup sera iniciado assim que assinatura for removida\n";
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
    }
   
    return 0;
}
