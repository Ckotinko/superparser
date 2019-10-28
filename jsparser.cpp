#include "jsparser.h"
#include <mutex>
using namespace ckotinko::lang;

static graph js;

jsgraph::jsgraph(config &cfg)
    : base_parser(cfg)
{
    static std::once_flag once;
    //собственно, почти весь синтаксис "обычного" JS укладывается в пару страниц:
    std::call_once(once, []() {
        //dont implement this: will handle as special case
        //*expression >> "//" >> (token_type::comment_token) >> 0;
        //*expression >> "/*" >> (token_type::comment_token) >> "*/";

        graph::iterator expression = js.at("expression");
        graph::iterator scope = js.at("scope");
        graph::iterator eos = js.at(";");
        graph::iterator func = js.at("func");
        eos >> ";" >> 0;
        {
            graph::iterator arg= (*func >> "(");
            graph::iterator body = (*arg >> ")");
            *body >> "{" >> scope() >> "}" >> 0;
            *arg >> (token_type::identifier_token, "arg_name") >> ")" >> body;
            *arg >> (token_type::identifier_token, "arg_name") >> "," >> arg;
        }
        {
            graph::iterator op2 = js.at("op2");
            //-10, !(a+b)
            *expression >> (token_type::operator_token, "prefix_operator") >> expression;

            //function
            *expression >> "function" >> func() >> 0;

            //10, 1.0
            *expression >> token_type::float_token >> op2;
            *expression >> token_type::integer_token >> op2;
            *expression >> (token_type::identifier_token, "ident") >> op2;


            //objects
            graph::iterator oelt = js.at("objel");
            graph::iterator obj  = (*expression >> "{");
            *obj >> (token_type::integer_token, "object_member") >> oelt;
            *obj >> (token_type::identifier_token, "object_member") >> oelt;
            *obj >> "}" >> 0;
            *oelt >> ":" >> expression() >> "}" >> 0;
            *oelt >> ":" >> expression() >> "," >> obj;

            //arrays
            graph::iterator aelt= js.at("arrel");
            graph::iterator arr = (*expression >> "[") >> aelt;
            *arr >> expression() >> aelt;
            *arr >> "]" >> 0;
            *aelt >> "," >> arr;
            *aelt >> "]" >> 0;

            //"blah", 'blah', `blah`
            *expression >> "\"" >> (token_type::string_token) >> "\"" >> op2;
            *expression >> "'"  >> (token_type::string_token) >> "'"  >> op2;
            *expression >> "/"  >> (token_type::string_token) >> "/"  >> op2;
            *expression >> "`"  >> (token_type::string_token) >> "`"  >> op2;

            // a
            *expression >> token_type::identifier_token >> op2;

            // (a+b)
            *expression >> "(" >> expression() >> ")" >> op2;
            // + * +=
            *op2 >> (token_type::operator_token, "binary_operator") >> expression;
            *op2 >> (token_type::operator_token, "suffix_operator") >> 0;
            *op2 >> 0;
            *op2 >> "[" >> expression() >> "]" >> op2;

            // b() c(1,2)
            graph::iterator as = (*op2 >> "(");
            graph::iterator ae = (*as  >> ")");
            *as >> expression() >> ")" >> ae;
            *as >> expression() >> "," >> as;
            *ae >> op2;
        }
        graph::iterator scope = js.at("scope");
        {
            //function f(arg1,arg2) { };
            graph::iterator as = (*scope >> "function" >> (token_type::identifier_token, "function_name")
                   >> "(");
            graph::iterator fendargs = (*fargs >> ")");
            *fargs >> (token_type::identifier_token, "arg_name") >> ")" >>fendargs;
            *fargs >> (token_type::identifier_token, "arg_name") >> "," >>fargs;
            *fendargs >> "{" >> scope() >> "}" >> eos;



            //var x;
            //var x,y;
            //var z=@expr, w;
            //let a=...;
            graph::iterator var      = js.at("var");
            *scope >> "var" >> var;
            *scope >> "let" >> var;
            graph::iterator val = (*var >> (token_type::identifier_token, "var_name"));
            *val >> "," >> (token_type::identifier_token, "var_name") >> val;
            *val >> "=" >> expression() >> val;
            *val >> eos;

            //(a.b.c = function() {...})(1,2);
            *scope >> expression() >> eos;
        }
    });


    {


            op2 | end

        );
        *expression >> (token_type::identifier_token, "identifier") >> op2;
        *op2 >> (token_type::operator_token, "binary_operator") >> expression;
        *op2 >> "." >> (token_type::identifier_token, "member") >> op2;
        *op2 >> "(" >> expression() >> ")" >> op2;
        *op2 >> "[" >> expression() >> "]" >> op2;

        iterator subexpression = at("subexpression");
        *expression >> "(" >> subexpression;

    }
    {
        iterator name = at("variable_name");
        *scope >> "let" >> name;
        *scope >> "var" >> name;

        *name >> "=" >> expression;
        name.end();


        *scope >> "function" >> token_type::identifier_token
    }
    (scope >> "var")("declare_variable")
}
