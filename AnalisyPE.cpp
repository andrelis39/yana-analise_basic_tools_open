//g++ AnalisyPE.cpp -o AnalisyPE
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

struct IMAGE_DOS_HEADER {
    uint16_t e_magic;    // "MZ" signature
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    int32_t  e_lfanew;   // Offset para o PE Header
};

struct IMAGE_FILE_HEADER {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
};

int main() {
    const char* filename = "C:\\caminho\\para\\seu.exe";

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Erro ao abrir o arquivo!\n";
        return 1;
    }

    IMAGE_DOS_HEADER dosHeader;
    file.read(reinterpret_cast<char*>(&dosHeader), sizeof(dosHeader));

    if (dosHeader.e_magic != 0x5A4D) { // "MZ"
        std::cerr << "Não é um arquivo PE válido!\n";
        return 1;
    }

    // Move para o PE Header
    file.seekg(dosHeader.e_lfanew, std::ios::beg);

    uint32_t peSignature;
    file.read(reinterpret_cast<char*>(&peSignature), sizeof(peSignature));
    if (peSignature != 0x00004550) { // "PE\0\0"
        std::cerr << "Assinatura PE inválida!\n";
        return 1;
    }

    IMAGE_FILE_HEADER fileHeader;
    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));

    std::cout << "Máquina: 0x" << std::hex << fileHeader.Machine << "\n";
    std::cout << "Número de seções: " << std::dec << fileHeader.NumberOfSections << "\n";
    std::cout << "Tamanho do Optional Header: " << fileHeader.SizeOfOptionalHeader << " bytes\n";
    std::cout << "Características: 0x" << std::hex << fileHeader.Characteristics << "\n";

    file.close();
    return 0;
}
