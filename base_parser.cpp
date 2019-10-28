#include "base_parser.h"
#include "misc.h"
#include <string.h>
#include <cassert>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBICU
#include <unicode/uchar.h>
#else
#include <cwctype>
#endif

using namespace ckotinko::lang;

base_parser::base_parser(const graph &g_, const config &cfg_)
    :g(g_), cfg(cfg_)
{
}
base_parser::~base_parser()
{
}
void base_parser::reset()
{
    state = state_t::initial;
    linecount = 1;
    columncount = 1;
}

enum { IDENT = 1, NUMERIC = 2, PUNCT = 4, SPACE = 8 };
static unsigned get_unichar_category(unsigned c1, unsigned c2) {
    unsigned q = 0;
    if (c1 < 128)
    {
        if (isspace(c1))
            q |= SPACE;
        if (isalpha(c1))
            q |= IDENT;
        if (c1 == '_' || c1 == '$')
            q |= IDENT;
        if (isdigit(c1))
            q |= NUMERIC;
        if (ispunct(q))
            q |= PUNCT;
    }
    else
    {
        unsigned c = c1;
        if (c2) c = 0x10000 + (c1 & 0x3ff) + ((c2 & 0x3ff)<<10);
#ifdef HAVE_LIBICU
        auto cat = u_charType(c);
        switch(cat) {
        case U_UPPERCASE_LETTER:
        case U_LOWERCASE_LETTER:
        case U_TITLECASE_LETTER:
            q = IDENT;break;
        case U_MATH_SYMBOL:
            q = PUNCT;break;
        default:;
        }
#else
        if (iswalpha(c))
            q |= IDENT;
        if (iswpunct(c))
            q |= PUNCT;
#endif
    }
    return q;
}

//gcc внезапно не может оптимально скомпилить вот это:
//если нет -O1 или версия больше 7
unsigned tohex(unsigned i, unsigned c) {
    unsigned char cc = (unsigned char)c;
    unsigned char d2 = ((cc < '9')-1) & ('A'-'0'-10);
    unsigned char dc = ((cc < 'a')-1) & ('a'-'A');
    cc -= dc + '0';//may overflow, if <'0' but it is ok.
    //32+ are invalid chars
    unsigned char dm = ((cc < 32)-1);
    unsigned char em = (int(~0x7E03FF00) >> (cc+1)) & 0x80;
    //+1 to move bit from bitmask to bit8
    unsigned char q  = (cc-d2) | em | dm;
    return (i<<4) | q | ((q < 16)-1);
}

void base_parser::add(unsigned c1, unsigned c2)
{
    assert(c2 == 0 || ((c1 & 0xD800)==0xD800 && (c2 & 0xDC00)==0xDC00));

    unsigned nl = (c1=='\n')? linecount+1:linecount;
    unsigned nc = (c1=='\n')? 0:columncount+((c1=='\t'?cfg.tabsize:1));
    bool noctrl = (c1 == '\n') | (c1 == '\r') | (c1 == '\t');
    bool nv     = (c1 < 128 ? iscntrl(c1):false) & !noctrl;

    switch (state) {
    case state_t::comment_sl: {
        if (c1 == '\n')
            emitToken(nl, nc);
        break;}
    case state_t::comment_ml:
    case state_t::string_raw: {
        assert(tparameter < aux.size());
        if (state == state_t::string_raw)
            add2(c1,c2);
        if (aux[tparameter++] != c1) {
            tparameter = 0;
            break;
        }
        if (__glibc_unlikely(c2)) {
            assert(tparameter < aux.size());
            if (aux[tparameter++] != c2) {
                tparameter = 0;
                break;
            }
        }
        if (tparameter == aux.size()) {
            assert(tok.size() >= aux.size());
            tok.resize(tok.size()-aux.size());
            emitToken(nl,nc);
        }
        break;}
    case state_t::string: {
        assert(aux.empty() == false);
        if (nv)
            throw std::runtime_error("invalid character");

        switch (tparameter) {//escaped
        case '\\':{
            switch(c1) {
            case '\n':
                tparameter = 0;
                break;
            case '\r':
            case 'c':
            case 'x'://x,y,z
                tparameter = c1;
                xparameter = 0;
                break;
            case '0' ... '7':
                tparameter = '1';//1,2,3
                xparameter = c1 - '0';
                break;
            case 'u':
                tparameter = 200;
                xparameter = 0;
                break;
            case 't':
            case 'b':
            case '\\':
                add(c1,0);
                tparameter = 0;
                break;
            default:
                if (c1 != sparameter) {
                    std::u16string m; m.append(1, c1);
                    if (c2) m.append(1, c2);
                    warning(std::make_pair(linecount, columncount-1),
                            std::make_pair(nl, nc),
                            "unknown escape");
                }
                add(c1,c2);
            }
            break;}
        case '\r':{
            tparameter = 0;
            if (c1 == '\n') break;
            goto just_add_characters;}
        case '1':{
            //match /n,/nn /nnn
            if (c1 >= '0' && c1 <= '7') {
                xparameter = (c1-'0')|(xparameter<<3);
                tparameter = '2';
                break;
            }
            add2(xparameter,0);
            goto just_add_characters;}
        case '2':{
            //match /n,/nn /nnn
            if (c1 >= '0' && c1 <= '7') {
                xparameter = (c1-'0')|(xparameter<<3);
                tparameter = 0;
                add2(xparameter,0);
                break;
            }
            add2(xparameter,0);
            goto just_add_characters;}
        case 'x':case 'y':{
            xparameter = tohex(xparameter, c1);
            if (xparameter == ~0u) {
                warning(std::make_pair(linecount, columncount-(tparameter-'x'+2)),
                        std::make_pair(nl, nc),
                        "invalid escape");
                goto just_add_characters;
            }
            if (++tparameter=='z') {
                add2(xparameter, 0);
                tparameter = 0;
            }
            break;}
        case 200:case 201:case 202:case 203:{
            xparameter = tohex(xparameter, c1);
            if (xparameter == ~0u) {
                warning(std::make_pair(linecount, columncount-(tparameter-202)),
                        std::make_pair(nl, nc),
                        "invalid escape");
                goto just_add_characters;
            }
            if (++tparameter==204) {
                add2(xparameter, 0);
                tparameter = 0;
            }
            break;}
        case 'c': {
            tparameter = 0;
            switch(c1) {
            case 'A' ... 'Z':
                add2(c1 - 'A' + 1, 0);
                break;
            case 'a' ... 'z':
                add2(c1 - 'a' + 1, 0);
                break;
            default:
                warning(std::make_pair(linecount, columncount-2),
                        std::make_pair(nl, nc),
                        "invalid escape");
                goto just_add_characters;
            }
            break;}
        case 0:just_add_characters:{
            tparameter = 0;
            if (c1 == sparameter) {
                emitToken(nl,nc);
                break;
            }
            if (c1 == '\\') {
                tparameter = c1;
                xparameter = 0;
                break;
            }
            add2(c1,c2);
            break;}
        default:
            throw std::logic_error("invalid state:" LINE_STRING);
        }
        break;}
    case state_t::numeric_0:__numeric_0:{
        switch(c1) {
        case 'x':case 'X'://hex
        case 'b':case 'B'://binary
            add2(c1,c2);
            notafloat = true;
            state = state_t::numeric_ni;
            break;
        case '.':
            add2(c1,c2);
            state = state_t::numeric_float;
            break;
        default:
            state = state_t::numeric_n;
            goto __numeric_n;
        }
        break;}
    /*case state_t::numeric_0x:{
        switch(c1) {
        case '+':case '-':
            pending_error = "cannot use + or - after E";
            state = state_t::numeric_bad;
            goto __numeric_bad;
        default:
            state = state_t::numeric_ni;
            goto __numeric_ni;
        }
        break;}*/
    case state_t::numeric_ni:{
        switch(c1) {
        case '0' ... '9':
        case 'a' ... 'f':
        case 'A' ... 'F':
            add2(c1,c2);
            break;
        case '\'':
            //C++14 feature: ignore '
            break;
        default:
            state = state_t::numeric_suf;
            goto __numeric_suffix;
        }
        break;}
    case state_t::numeric_n:__numeric_n:{
        switch(c1) {
        case '0' ... '9':
            add2(c1,c2);
            break;
        case '\'':
            //C++14 feature: ignore '
            break;
        case '.':
            add2(c1,c2);
            state = state_t::numeric_float;
            break;
        case 'e':case 'E':
            isfloat = true;
            add2(c1,c2);
            state = state_t::numeric_f_e;
            break;
        default:
            state = state_t::numeric_suf;
            goto __numeric_suffix;
        }
        break;}
    case state_t::numeric_float: {
        isfloat = true;
        switch(c1) {
        case '0' ... '9':
            add2(c1,c2);
            break;
        case '\'':
            //C++14 feature: ignore '
            break;
        case 'e':case 'E':
            add2('e',0);
            state = state_t::numeric_f_e;
            break;
        default:
            state = state_t::numeric_suf;
            goto __numeric_suffix;
        }
        break;}
    case state_t::numeric_f_e:{
        switch(c1) {
        case '+':
        case '-':
            add(c1,0);
            state = state_t::numeric_f_e_s;
            break;
        case '0' ... '9':
            add('+',0);
            state = state_t::numeric_f_exp;
            goto __numeric_f_exp;
        default:
            pending_error = "expected floating point exponent after E";
            state = state_t::numeric_bad;
            goto __numeric_bad;
        }
        break;}
    case state_t::numeric_f_e_s:{
        switch(c1) {
        case '0' ... '9':
            add('+',0);
            state = state_t::numeric_f_exp;
            goto __numeric_f_exp;
        default:
            pending_error = "expected floating point exponent after E";
            state = state_t::numeric_bad;
            goto __numeric_bad;
        }
        break;}
    case state_t::numeric_f_exp:__numeric_f_exp:{
        switch(c1) {
        case '0' ... '9':
            add(c1,0);
            break;
        default:
            state = state_t::numeric_suf;
            goto __numeric_suffix;
        }
        break;}
    case state_t::numeric_suf:__numeric_suffix:{
        if (!cfg.accept_numeric_suffixes) {
            pending_error = "numeric suffixes are not available";
            state = state_t::numeric_bad;
            goto __numeric_bad;
        }

        switch(c1) {
        case 'i':case 'I':
            if (cfg.cxx < 11) {
                pending_error = "im-suffixes are available only in C++11";
                state = state_t::numeric_bad;
                goto __numeric_bad;
            }
            if (notafloat) {
                pending_error = "im-suffixes are applicable only to floating point numbers";
                state = state_t::numeric_bad;
                goto __numeric_bad;
            }
            isfloat = true;
            goto __sfx;
            //fall-through
        case 'f':case 'F':
        case 'u':case 'U':
        case 'l':case 'L':__sfx:
            suffix = (suffix<<8)|(c1|0x20);
            if (suffix>0xffffffu) {
                pending_error = "invalid suffix";
                state = state_t::numeric_bad;
                goto __numeric_bad;
            }
            break;
        default:
            switch(suffix) {
            case 0:break;
            case ('i')://i
            case ('i'+'f'*256)://if
            case ('i'+'l'*256)://il
            case ('u'):
            case ('u'+'l'*256):
            case ('u'+'l'*256+'l'*65536):
            case ('l'):
            case ('l'+'u'*256):
            case ('l'+'l'*256):
            case ('l'+'l'*256+'u'*65536):
                break;
            default:
                pending_error = "invalid suffix";
                state = state_t::numeric_bad;
                goto __numeric_bad;
            }
            state = state_t::numeric_after;
            goto __numeric_after;
        }
        break;}
    case state_t::numeric_after:__numeric_after:{
        if (c1 == '_') {
            if (cfg.cxx < 11) {
                pending_error = "user defined suffixes are available only in C++11";
                state = state_t::numeric_bad;
                goto __numeric_bad;
            }
            emitToken(nl,nc);
            state = state_t::user_defined_suffix;
            break;
        }
        if (!(get_unichar_category(c1,c2) & (IDENT|NUMERIC))) {
            emitToken(linecount,columncount);
            goto __initial;
        }
        pending_error = "invalid characters at end of numeric constant";
        state = state_t::numeric_bad;
        goto __numeric_bad;}
    case state_t::numeric_bad:__numeric_bad:{
        if (!(get_unichar_category(c1,c2) & (IDENT|NUMERIC))) {
            emitToken(linecount,columncount);
            goto __initial;
        }
        break;}
   case state_t::user_defined_suffix:
   case state_t::identifier:{
        if (get_unichar_category(c1, c2) & (IDENT|NUMERIC)) {
            add2(c1,c2);
            break;
        }
        emitToken(nl,nc);
        goto __initial;}
    case state_t::dot:{
        if (c1 >= '0' || c1 <= '9') {
            add2('0',0);
            add2('.',0);
            add2(c1,0);
            state = state_t::numeric_float;
            break;
        }
        //ok, it was operator.
        tok.push_back('.');
        if (matchToken(nl,nc))
            goto __initial;
        state = state_t::operator2;
        goto __operator2;}
    case state_t::operator2:__operator2:{
        switch(c1) {
        case '~':case '!':case '%':case '^':case ':':case '&':case ';':
        case '?':case '*':case '(':case ')':case '-':case '+':case '=':
        case '[':case ']':case '{':case '}':case '@':case '<':case '>':
        case ',':case '|':case '\'':case '\"':case '/':case '.':
            tok.push_back(c1);
            matchToken(nl,nc);
            break;
        default:
            pending_error = "invalid operator";
            state = state_t::error_state;
            emitToken(nl,nc);
        }
        break;}
    case state_t::initial:__initial:{
        unsigned cat;
        memset(&blob, 0, sizeof(blob));
        aux.clear();
        tok.clear();
        pending_error.clear();

        switch(c1) {
        case ' ':case '\t':case '\r':case '\n':
            //white space
            break;
        case '~':case '!':case '%':case '^':case ':':case '&':case ';':
        case '?':case '*':case '(':case ')':case '-':case '+':case '=':
        case '[':case ']':case '{':case '}':case '@':case '<':case '>':
        case ',':case '|':case '\'':case '\"':case '/':
            tok.push_back(c1);
            if (!matchToken(nl,nc))
                state = state_t::operator2;
            break;
        case '.'://tricky part: can be numeric or operator!
            //thanks to assholes who made it possible in C
            //detect if current vertex supports operator matching
            //if operator is expected, '.' is an operator
            //otherwise, '.' is a beginning of float
            state = state_t::dot;
            break;
        case '0':
            state = state_t::numeric_0;
            goto __numeric_0;
        case '1' ... '9':
            state = state_t::numeric_n;
            goto __numeric_n;
        case 'a' ... 'z':
        case 'A' ... 'Z':
        case '_':case '$':
            tok.push_back(c1);
            state = state_t::identifier;
            break;
        default:
            cat = get_unichar_category(c1, c2);
            if (cat & SPACE)
                break;
            if (cat & IDENT) {
                add(c1,c2);
                state = state_t::identifier;
                break;
            }
            if (cat & PUNCT) {
                tok.push_back(c1);
                if (!matchToken(nl,nc))
                    state = state_t::operator2;
                break;
            }
            pending_error = "invalid character";
            state = state_t::error_state;
            emitToken(nl,nc);//will reset state back
        }
        break;}
    default:
        throw std::logic_error("invalid state " LINE_STRING);
    }
    linecount = nl;
    columncount = nc;
}

void base_parser::add2(unsigned c1, unsigned c2)
{
    tok.push_back(c1);
    if (c2) tok.push_back(c2);
}

void base_parser::emitToken(unsigned endrow, unsigned endcol)
{
    switch(state) {
    case state_t::comment_sl:
    case state_t::comment_ml:
        onToken(token_type::comment_token,
                std::make_pair(linecount, columncount),
                std::make_pair(endrow, endcol),
                tok);
        break;
    case state_t::string_raw:
    case state_t::string:
        onToken(token_type::string_token,
                std::make_pair(linecount, columncount),
                std::make_pair(endrow, endcol),
                tok);
        break;
    case state_t::numeric_after:
        if (isfloat) {
            onToken(token_type::float_token,
                    std::make_pair(linecount, columncount),
                    std::make_pair(endrow, endcol),
                    tok, suffix);
        } else {
            onToken(token_type::integer_token,
                    std::make_pair(linecount, columncount),
                    std::make_pair(endrow, endcol),
                    tok, suffix);
        }
        break;
    case state_t::identifier:
        onToken(token_type::identifier_token,
                std::make_pair(linecount, columncount),
                std::make_pair(endrow, endcol),
                tok);
        break;
    case state_t::user_defined_suffix:
        onToken(token_type::user_suffix_token,
                std::make_pair(linecount, columncount),
                std::make_pair(endrow, endcol),
                tok);
        break;
    case state_t::numeric_bad:
    case state_t::error_state:
        //handle bad token
        error(std::make_pair(linecount, columncount),
              std::make_pair(endrow, endcol),
              pending_error);
        break;
    default:
        throw std::logic_error("invalid state " LINE_STRING);
    }
    //linecount   = token_end_line;
    //columncount = token_end_col;
    state = state_t::initial;
}
bool base_parser::matchToken(unsigned endrow, unsigned endcol)
{

    bool match = onToken(token_type::operator_token,
                         std::make_pair(linecount, columncount),
                         std::make_pair(endrow, endcol),
                         tok);
    if (match) state = state_t::initial;
    return match;
}
