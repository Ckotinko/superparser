#include "parser_utf16.h"

using namespace ckotinko::lang;

parser_utf16::parser_utf16(const graph &g_, const config &cfg_, bool bom_)
    : base_parser(g_, cfg_)
{
    if (bom_) bom = 16;
}
parser_utf16::~parser_utf16()
{
}
void parser_utf16::input(const char *data, size_t size, size_t &consumed)
{
    size_t i = 0;
    try {
        while(i < size)
        {
            unsigned c = data[i++];
            switch(bom) {
            case 0:
                if (c == 0xFF) {//LE
                    bom = 1;
                    break;
                }
                if (c == 0xFE) {
                    bom = 6;
                    break;
                }
                bom = 16;goto default_bom;
            case 1:
                if (c != 0xFE)
                    goto bad_bom;
                bom = 16;
                break;
            case 6:
                if (c != 0xFF)
                    goto bad_bom;
                bom = 61;
                break;
            default:
            default_bom:
                if (__glibc_unlikely(accum)) {
                    c = (0xff&accum<<8) | c;
                    accum = 0;
                } else if (__glibc_unlikely(i == size)) {
                    accum = c | 0x100;
                    break;
                } else
                    ++i;

                if (bom == 61)
                    c = __builtin_bswap16(c);

                if (__glibc_unlikely(combining)) {
                    if (__glibc_unlikely((c & 0xDC00u) != 0xDC00u))
                        throw std::runtime_error("unvalid utf16 character");
                    add(combining, c);
                    combining = 0;
                } else if (__glibc_unlikely((c & 0xD800u) == 0xD800u)) {
                    combining = c;
                } else {
                    add(c, 0);
                }
                break;
            bad_bom:
                throw std::runtime_error("invalid byte order mark");
            }
        }
    } catch(...) {
        consumed = i;
        throw;
    }
    consumed = i;
}
void parser_utf16::finish()
{
    if (combining | accum)
        throw std::runtime_error("incomplete character at end of stream");
    base_parser::finish();
}
