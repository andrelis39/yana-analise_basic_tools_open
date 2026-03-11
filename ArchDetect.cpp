//g++ ArchDetect.cpp -o ArchDetect / For linux
//x86_64-w64-mingw32-g++ ArchDetect.cpp -o ArchDetect  /For windows
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <iostream>

typedef enum {
    FILE_UNKNOWN,
    FILE_PE_X86,
    FILE_PE_X64,
    FILE_ELF_X86,
    FILE_ELF_X64
} FileType;

FileType DetectFileType(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f) return FILE_UNKNOWN;

    unsigned char buffer[64];
    fread(buffer, 1, sizeof(buffer), f);

    // ----- Check ELF -----
    if (buffer[0] == 0x7F &&
        buffer[1] == 'E' &&
        buffer[2] == 'L' &&
        buffer[3] == 'F')
    {
        if (buffer[4] == 1)
            return FILE_ELF_X86;
        else if (buffer[4] == 2)
            return FILE_ELF_X64;
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
                return FILE_PE_X86;
            else if (machine == 0x8664)
                return FILE_PE_X64;
        }
    }

    fclose(f);
    return FILE_UNKNOWN;
}
int main(int argc, const char* argv[])
{

    if(argc < 2)
    {
        std::cout << "Use > " << argv[0] << " <programName>\n";
        return 1;
    }
    FileType type = DetectFileType(argv[1]);

    switch (type)
    {
        case FILE_PE_X86:
            printf("Format : Portable Executable (PE)\n");
            printf("Arch   : x86 (32-bit)\n");
            break;

        case FILE_PE_X64:
            printf("Format : Portable Executable (PE)\n");
            printf("Arch   : x86-64 (64-bit)\n");
            break;

        case FILE_ELF_X86:
            printf("Format : Executable and Linkable Format (ELF)\n");
            printf("Arch   : x86 (32-bit)\n");
            break;

        case FILE_ELF_X64:
            printf("Format : Executable and Linkable Format (ELF)\n");
            printf("Arch   : x86-64 (64-bit)\n");
            break;

        default:
            printf("Format : Unknown\n");
            printf("Arch   : Unknown\n");
            break;
    }
}