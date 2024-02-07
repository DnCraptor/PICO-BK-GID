#pragma once
extern "C" {
    #include <ff.h>
}
#include <string>
namespace fs {

class path {
    std::string p;
public:
    path() {}
    path(const char* _p) : p(_p) {}
    path(const std::string& _p) : p(_p) {}
    ~path() {}
    friend path operator / (const path& p1, const path& p2) {
        if (p1.p.at(p1.p.size() - 1) == '\\')
           return path(p1.p + p2.p);
        return path(p1.p + "\\" + p2.p);
    }
    const char* c_str() const { return p.c_str(); }
    void clear() { return p.clear(); }
    bool empty() const { return p.empty(); }
    bool has_root_name() const { return p.at(0) == '\\'; }
    bool has_parent_path() const {
        return 0 != strstr(p.c_str(), "\\");
    }
    bool has_extension() const {
        const char * pp = strstr(p.c_str(), "\\");
        return 0 != strstr(pp ? pp : p.c_str(), ".");
    }
    path stem() const {
        // TODO:
        return *this;
    }
    void replace_extension() {
        const char * pp = strstr(p.c_str(), "\\");
        pp = strstr(pp ? pp : p.c_str(), ".");
        if (pp != 0)
            p[pp - p.c_str()] = 0;
    }
    void replace_extension(const std::string& r) {
        const char * pp = strstr(p.c_str(), "\\");
        pp = strstr(pp ? pp : p.c_str(), ".");
        if (pp != 0)
            p[pp - p.c_str()] = 0;
        p += "." + r;
    }
    path filename() const {
        std::string s = p;
        const char * pp = strstr(s.c_str(), "\\");
        pp = strstr(pp ? pp : s.c_str(), ".");
        if (pp != 0)
            s[pp - p.c_str()] = 0;
        return path(s);
    }
    path extension() const {
        const char * pp = strstr(p.c_str(), "\\");
        pp = strstr(pp ? pp : p.c_str(), ".");
        if (pp) return path(pp + 1);
        return path();
    }
    std::string string() const { return p; }
};

inline static bool exists(const path& p) {
    return f_stat(p.c_str(), 0) == FR_OK;
}

};
