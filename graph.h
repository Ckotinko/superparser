#pragma once
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <ostream>
#include <boost/serialization/split_member.hpp>
#include "function.h"

namespace ckotinko {
namespace lang {

enum token_type : unsigned {
    identifier_token,
    operator_token,
    integer_token,
    uinteger_token,
    float_token,
    string_token,
    comment_token,
    user_suffix_token,
    max_token_type
};

typedef function<void ()> callback_t;
typedef function<bool (const std::string&)> edge_check_t;

class base_parser;
class base_parser_u16;
class graph {
public:
    virtual ~graph();
    class  iterator;
    friend class iterator;
    friend class base_parser;
    friend class base_parser_u16;
private:
    enum : unsigned { invalid_id = ~0u };
    struct edge_data {
        template<class Archive>
        void save(Archive & ar, const unsigned int version)
        {
            ar & type;
            ar & target_vertex;
            ar & parameter;
            ar & callback_id;
        }
        template <class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & type;
            ar & target_vertex;
            ar & parameter;
            ar & callback_id;
            callback = edge_check_t();
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()

        unsigned     type;
        unsigned     target_vertex = invalid_id;
        unsigned     parameter     = 0;
        std::string  callback_id;
        edge_check_t callback;
    };
    struct vertex_data {
        template<class Archive>
        void save(Archive & ar, const unsigned int version)
        {
            ar & callin;
            ar & invalid_id;
            ar & terminal;
            ar & edges;
            ar & callback_id;
        }
        template <class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & callin;
            ar & invalid_id;
            ar & terminal;
            ar & edges;
            ar & callback_id;
            callback = callback_t();
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        unsigned    callin          = invalid_id;
        unsigned    defaultto       = invalid_id;
        bool        terminal        = false;
        std::list<edge_data> edges;
        std::string callback_id;
        callback_t  callback;
    };
public:
    graph() {}

    struct filter {
        filter(token_type tt, const std::string& id)
            : type(tt), callback_id(id) {}
        filter(token_type tt, const char* id)
            : type(tt), callback_id(id) {}
        token_type   type;
        std::string callback_id;
    };
    class iterator {
    public:
        struct __call { unsigned v; };
        //начать строить цепочку заново с начала ветки
        iterator& operator *();
        //добавить токен "как есть"
        iterator& operator >>(const std::string& token);
        iterator& operator >>(const char * token);
        //узнать токен по типу(кроме exact_match)
        iterator& operator >>(token_type tt);
        //то же самое но использовать фильтр. правильно вызывать так: >>(token_type::xx, "callback_name")>>
        iterator& operator >>(const filter& filter);//internal
        //выполнить вызов вложенного графа. правильно вызывать так: >>iterator()>>
        __call    operator()();
        iterator& operator >>(const __call& callv);//internal
        //узнать последовательность типа raw string: >>"R\"">>3>>"\"">> узнает R"xxxasdgdfgxxx" как asdgdfg,
        //                            но границы будут как у R"xxxasdgdfgxxx"
        //исключение: n = 0 == установить признак выхода из вызова
        iterator& operator >>(unsigned n);//fixed size match, then match until next same sequence -> raw strings
        //примкнуть к началу другой ветки
        void      operator >>(const iterator& to);
        //установить обработчик вручную
        iterator& operator()(const std::string& callback);
    private:
        friend class graph;
        iterator(graph& g_, unsigned v_, edge_data * e_=nullptr)
            : g(g_),e(e_),b(v_),v(v_) {}
        void completeEdge();

        graph     &g; //граф который мы строим
        edge_data *e; //текущая неоконченая дуга
        unsigned   b; //начальная вершина пути
        unsigned   v; //текущая вершина пути, может быть invalid_id если e!=0
    };

    template<class Archive>
    void save(Archive & ar, const unsigned int version)
    {
        ar & root_vertices;
        ar & all_tokens;
        ar & all_vertices;
    }
    template <class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        ar & root_vertices;
        ar & all_tokens;
        ar & all_vertices;
        verify();
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    iterator at(const std::string& id);
    iterator at(const char * id);
    iterator end();

private:
    bool equal(const graph& g) const { return this == &g; }
    std::unordered_map<std::string,unsigned> root_vertices;
    std::unordered_map<std::string,unsigned> all_tokens;
    std::vector<vertex_data>                 all_vertices;
    void verify();
};

}
}

ckotinko::lang::graph::filter operator ,(ckotinko::lang::token_type t,
                                         const std::string& callback_id)
{
    return ckotinko::lang::graph::filter(t,callback_id);
}
