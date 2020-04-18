#include <vector>
#include <string>
#include <iostream>
using namespace std;

namespace Misc {
    std::vector<unsigned char> concat() {
        return {};
    }

    template<class ... Types>
    std::vector<unsigned char> concat(string s, Types... args);
    template<class ... Types>
    std::vector<unsigned char> concat(int s, Types... args);
    template<class ... Types>
    std::vector<unsigned char> concat(vector<unsigned char> s, Types... args);
    template<class ... Types>
    std::vector<unsigned char> concat(unsigned char s, Types... args);
    template<class ... Types>
    std::vector<unsigned char> concat(const unsigned char* s, Types... args);

    template<class ... Types>
    std::vector<unsigned char> concat(string s, Types... args) {
        vector<unsigned char> ret(s.begin(), s.end());
        auto rest = concat(args...);
        ret.insert(ret.end(), rest.begin(), rest.end());
        return ret;
    }

    template<class ... Types>
    std::vector<unsigned char> concat(int s, Types... args) {
        unsigned char* p = (unsigned char*)&s;
        vector<unsigned char> ret{*p, *(p+1), *(p+2), *(p+3)};
        // cout << (unsigned char)*p << endl;
        vector<unsigned char> rest = concat(args...);
        ret.insert(ret.end(), rest.begin(), rest.end());
        return ret;
    }

    template<class ... Types>
    std::vector<unsigned char> concat(vector<unsigned char> s, Types... args) {
        vector<unsigned char> ret(s.begin(), s.end());
        vector<unsigned char> rest = concat(args...);
        ret.insert(ret.end(), rest.begin(), rest.end());
        return ret;
    }

    template<class ... Types>
    std::vector<unsigned char> concat(unsigned char s, Types... args) {
        vector<unsigned char> ret{s};
        vector<unsigned char> rest = concat(args...);
        ret.insert(ret.end(), rest.begin(), rest.end());
        return ret;
    }

    template<class ... Types>
    std::vector<unsigned char> concat(const char* s, Types... args) {
        std::string a(s);
        return concat(a, args...);
    }
}
