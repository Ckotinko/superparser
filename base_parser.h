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

    virtual callback_t   getCallback(const std::string& name) = 0;
    virtual edge_check_t getEdgeCallback(const std::string& name) = 0;
    virtual void         warning(unsigned row0,
                                 unsigned col0,
                                 unsigned row1,
                                 unsigned col1,
                                 const char * msg) = 0;
    virtual void         error(unsigned row0,
                               unsigned col0,
                               unsigned row1,
                               unsigned col1,
                               const char * msg) = 0;

    //valid only if accessed from a registered callback!
    token_type           token() const { return type; }
    const std::u16string&wstring() const { return tok; }
    unsigned             parameter() const { return tparameter; }
    std::pair<unsigned,unsigned> token_start() const { return std::make_pair(token_start_line, token_start_col); }
    std::pair<unsigned,unsigned> token_end() const { return std::make_pair(token_end_line, token_end_col); }

    unsigned             line() const { return linecount; }
    unsigned             column() const { return columncount; }
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
        numeric_bad,    //consume symbols which are considered invalid numeric
        numeric_suf,    //+u +l +f +i _ud

        user_defined_suffix,
        dot,

        identifier,     //

        operator2,   //

        bad_state,      //used to track invalid state
    };

    const graph&   g;
    config         cfg;
    unsigned       linecount   = 1;
    unsigned       columncount = 1;

    state_t        state = state_t::initial;
    //the following are filled only before calling a callback!
    token_type     type;
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
        };
        unsigned blob[8];
    };
    unsigned       token_start_line;
    unsigned       token_end_line;
    unsigned       token_start_col;
    unsigned       token_end_col;
    std::string    pending_error;

    void add2(unsigned c1, unsigned c2);
    void emitToken(unsigned endrow, unsigned endcol);
    bool matchToken(unsigned endrow, unsigned endcol);
};

}
}
