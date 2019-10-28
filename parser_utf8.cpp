#include "parser_utf8.h"

using namespace ckotinko::lang;

parser_utf8::parser_utf8(const graph &g_, const config &cfg_)
    :base_parser(g_,cfg_)
{
}
parser_utf8::~parser_utf8()
{
}
void parser_utf8::input(const char *data, size_t size, size_t &consumed)
{
    size_t i = 0;
    try {
        for(; i < size; ++i) {
            unsigned c = data[i];
            //обработка UTF8
            if (__glibc_unlikely(c >= 128))
            {
                if (c >= 0b11111000) {
                    throw std::runtime_error("invalid utf8");
                }
                if (c >= 0b11110000) {
                    if (unichar_left)
                        throw std::runtime_error("invalid utf8");
                    unichar_left = 3;
                    unichar      = c & 0x7;
                    continue;
                }
                if (c >= 0b11100000) {
                    if (unichar_left)
                        throw std::runtime_error("invalid utf8");
                    unichar_left = 2;
                    unichar      = c & 0xF;
                    continue;
                }
                if (c >= 0b11000000) {
                    if (unichar_left)
                        throw std::runtime_error("invalid utf8");
                    unichar_left = 1;
                    unichar      = c & 0x1F;
                    continue;
                }
                unichar = (unichar<<6) | (c & 0x3f);
                if (--unichar_left)
                    continue;
                c = unichar;
                unichar = 0;
            }
            if (c >= 0x10ffff || (c & 0xD800) == 0xD800 || (c & 0xDC00) == 0xDC00)
                throw std::runtime_error("unvalid utf8");
            if (c < 0x10000)
                add(c, 0);
            else {
                c -= 0x10000;
                add(0x3ff & c, 0x3ff & (c>>10));
            }
        }
    } catch(...) {
        consumed = i;
        throw;
    }
    consumed = i;
}
void parser_utf8::finish()
{
    if (unichar_left)
        throw std::runtime_error("incomplete character at end of stream");
    base_parser::finish();
}
