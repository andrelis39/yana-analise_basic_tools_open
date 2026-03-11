#include <array>
#include <string>
#include <windows.h>

template<size_t N>
class SecureString {
private:
    std::array<char, N> encrypted{};
    static constexpr char key = 0x5A;

public:
    constexpr SecureString(const char(&str)[N]) {
        for (size_t i = 0; i < N; i++)
            encrypted[i] = str[i] ^ key;
    }

    std::string decrypt() const {
        std::string result;
        result.resize(N - 1);

        for (size_t i = 0; i < N - 1; i++)
            result[i] = encrypted[i] ^ key;

        return result;
    }
};
//Use
constexpr SecureString ip("192.168.0.10");

int main() {

    std::string real_ip = ip.decrypt();

    // usa o IP aqui
    // connect(real_ip.c_str(), ...);

    SecureZeroMemory(real_ip.data(), real_ip.size());
}