//g++ genkey.cpp -o genkey
#include <string>
#include <iostream>
#include <fstream>

int main()
{
    std::ofstream file("serial/SID8.key", std::ios::out | std::ios::trunc);
    file << "F52AC10B-58CC-1522-A567-0E02B2C3D401\n";
    file << "8\n";
    file << "SERVER\n";
    file.close();
    /*
    std::ofstream file("serial/Serial.key", std::ios::out | std::ios::trunc);
    file << "F47AC10B-58CC-4372-A567-0E02B2C3D479\n"; //USB Backup
    file << "F52AC10B-58CC-1522-A567-0E02B2C3D401\n"; //USB Server
    file << "F47IC10O-58CC-4372-A557-0E02B2C3D440\n";
    file << "M47IC10O-58BB-4372-A557-0E02B2C3D258\n";
    file << "M47IC200-58FF-4372-F557-0E02B2C3D125\n";
    file.close();
    */

    return 0;
}