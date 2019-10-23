#pragma once
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <boost/serialization/split_member.hpp>
#include "function.h"

namespace ckotinko {
namespace lang {

enum token_type : unsigned {
    exact_match,
    identifier_token,
    operator_token,
    integer_token,
    uinteger_token,
    float_token,
    string_token,
    char_token,
    immediate_token,//matches integer,uinteger,float,string,char
    max_token_type
};

typedef function<void ()> callback_t;
typedef function<bool (const std::string&)> edge_check_t;

class context;
class u16context;
class graph {
public:
    class  iterator;
    friend class iterator;
    friend class context;
    friend class u16context;
private:
    enum : unsigned { invalid_id = ~0u };
    struct edge_data {
        template<class Archive>
        void save(Archive & ar, const unsigned int version)
        {
            ar & type;
            ar & target_vertex;
            ar & check_token;
            ar & callback_id;
        }
        template <class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & type;
            ar & target_vertex;
            ar & check_token;
            ar & callback_id;
            callback = edge_check_t();
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()

        unsigned     type;
        unsigned     target_vertex = invalid_id;
        unsigned     check_token   = invalid_id;
        std::string  callback_id;
        edge_check_t callback;
    };
    struct vertex_data {
        template<class Archive>
        void save(Archive & ar, const unsigned int version)
        {
            ar & callin;
            ar & terminal;
            ar & edges;
            ar & callback_id;
        }
        template <class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & callin;
            ar & terminal;
            ar & edges;
            ar & callback_id;
            callback = callback_t();
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        unsigned    callin          = invalid_id;
        bool        terminal        = false;
        std::list<edge_data> edges;
        std::string callback_id;
        callback_t  callback;
    };
public:    
    graph() {}
    virtual callback_t getCallback(const std::string& name) = 0;
    virtual edge_check_t getEdgeCallback(const std::string& name) = 0;

    class iterator {
    public:
        iterator& operator >>(const std::string& token);
        iterator& operator >>(const char * token);
        iterator& operator >>(token_type tt);
        iterator& operator()();
        iterator& operator()(const std::string& callback);
    private:
        friend class graph;
        iterator(graph& g_, unsigned v_, edge_data * e_)
            : g(g_),e(e_),v(v_) {}
        void completeEdge();
        graph     &g;
        edge_data *e;
        unsigned   v;
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
private:
    std::unordered_map<std::string,unsigned> root_vertices;
    std::unordered_map<std::string,unsigned> all_tokens;
    std::vector<vertex_data>                 all_vertices;
    void verify();
};

class context {
public:
private:

};

}
}
