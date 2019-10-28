#pragma once
#include <string>
#include "base_parser.h"

namespace ckotinko {
namespace lang {

class parser_utf16 : public base_parser {
public:
    virtual ~parser_utf16();
    void input(const char * data, size_t size, size_t &consumed);
    void finish();
protected:
    parser_utf16(const graph &g_, const config &cfg_, bool bom = false);
private:
    unsigned       bom       = 0;
    unsigned       combining = 0;
    unsigned       accum     = 0;
};

}
}
