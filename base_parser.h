#pragma once
#include <string>
#include "graph.h"
namespace ckotinko {
namespace lang {

class base_parser {
public:
    struct config {
        unsigned    cxx;//0,11,14,17... don't expect full support
        unsigned    tabsize;
        bool        accept_numeric_suffixes;//ulf
        unsigned    mode;//0,8,16 - encoding, 0 = auto 16bit
    };
    virtual void input(const char *data, size_t size, size_t &consumed) = 0;
    virtual void finish();
    virtual void reset();

    virtual bool         onToken(token_type type,
                                 std::pair<unsigned,unsigned> from,
                                 std::pair<unsigned,unsigned> to,
                                 const std::u16string& token,
                                 unsigned detail = 0) = 0;
    virtual void         warning(std::pair<unsigned,unsigned> from,
                                 std::pair<unsigned,unsigned> to,
                                 const std::string& msg) = 0;
    virtual void         error(std::pair<unsigned,unsigned> from,
                               std::pair<unsigned,unsigned> to,
                               const std::string& msg) = 0;

    //valid only if accessed from a registered callback!
    const std::u16string&wstring() const { return tok; }
    unsigned             parameter() const { return tparameter; }
    virtual ~base_parser();
protected:
    base_parser(const graph& g, const config& cfg);
    void add(unsigned c1, unsigned c2);
private:
    enum class state_t {
        initial,

        comment_ml,
        comment_sl,

        string,
        string_raw,

        numeric_0,      //0....
        numeric_0x,     //0x... 0b...
        numeric_ni,     //123... 012... 0x12... 0b12...
        numeric_n,      //123... 012... 0x12... 0b12...
        numeric_float,  //0.12 123.45
        numeric_f_e,    //12.34e
        numeric_f_e_s,  //12.34e+
        numeric_f_exp,  //12.34e+5
        numeric_after,  //check if next symbol is not a part of numeric
        numeric_suf,    //+u +l +f +i _ud

        numeric_bad,    //consume symbols which are considered invalid numeric

        user_defined_suffix,//c++11 feature(can be disabled)
        dot,            //decide if this is a float or an operator

        identifier,     //some identifier, but can be operator:it's up to client to decide

        operator2,      //operator consisting of more than one character

        error_state,    //used to track invalid state
    };

    const graph&   g;
    config         cfg;
    unsigned       linecount   = 1;
    unsigned       columncount = 1;

    state_t        state = state_t::initial;
    //the following are filled only before calling a callback!
    std::u16string tok;
    std::u16string aux;
    union {
        struct {//string parameters
            unsigned       tparameter;
            unsigned       sparameter;
            unsigned       xparameter;
        };
        struct {
            unsigned       suffix;
            bool           notafloat;
            bool           isfloat;
        };
        unsigned blob[8];
    };
    std::string    pending_error;

    void add2(unsigned c1, unsigned c2);
    void emitToken(unsigned endrow, unsigned endcol);
    bool matchToken(unsigned endrow, unsigned endcol);
};

}
}
