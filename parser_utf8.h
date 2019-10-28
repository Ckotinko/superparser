#pragma once
#include <string>
#include "base_parser.h"

namespace ckotinko {
namespace lang {

class parser_utf8 : public base_parser {
public:
    virtual ~parser_utf8();
    void input(const char * data, size_t size, size_t &consumed);
    void finish();
protected:
    parser_utf8(const graph &g_, const config &cfg_);
private:
    unsigned       unichar_left = 0;
    unsigned       unichar = 0;
};

}
}
