#pragma once
#include "base_parser.h"

namespace ckotinko {
namespace lang {

class jsgraph : public base_parser {
public:
    jsgraph(struct config& cfg);

private:
    bool onToken(
            token_type type,
            std::pair<unsigned, unsigned> from,
            std::pair<unsigned, unsigned> to,
            const std::u16string &token,
            unsigned detail) override;
    match_result matchToken(
            std::pair<unsigned, unsigned> from,
            std::pair<unsigned, unsigned> to,
            const std::u16string &token,
            std::u16string &se) override;
    void warning(
            std::pair<unsigned, unsigned> from,
            std::pair<unsigned, unsigned> to,
            const std::string &msg) override;
    void error(
            std::pair<unsigned, unsigned> from,
            std::pair<unsigned, unsigned> to,
            const std::string &msg) override;
};

}
}
