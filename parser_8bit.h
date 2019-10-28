#pragma once
#include <string>
#include "base_parser.h"

namespace ckotinko {
namespace lang {

class parser_8bit : public base_parser {
public:
    enum class encoding { ascii, cp1251, koi8 };
    virtual ~parser_8bit();
    void input(const char * data, size_t size, size_t &consumed);
protected:
    parser_8bit(const graph &g_, const config &cfg_, encoding e);
private:
    const uint16_t * cvt;
};

}
}
