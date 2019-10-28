#include "graph.h"
#include "misc.h"
#include <limits>
#include <stdexcept>
#include <cassert>
#include <cctype>

#define MAX_FIXED_MATCH 128

using namespace ckotinko::lang;

enum token_type2 : unsigned {
    exact_match = max_token_type, //internal
    fixed_size_match, //internal
    max_internal_token_type,
};

graph::~graph()
{

}

graph::iterator graph::at(const char *id)
{
    return at(std::string(id));
}

graph::iterator graph::at(const std::string &id)
{
    auto i = root_vertices.find(id);
    if (i != root_vertices.end())
        return iterator(*this, i->second, nullptr);

    size_t v = all_vertices.size();
    root_vertices[id] = v;

    all_vertices.emplace_back();
    return iterator(*this, v, nullptr);
}

void graph::iterator::completeEdge() {
    if (e) {
        if (e->target_vertex == invalid_id) {
            e->target_vertex = g.all_vertices.size();
            g.all_vertices.emplace_back();
        }
        v = e->target_vertex;
        e = nullptr;
    } else if (v == invalid_id)
        throw std::logic_error("iterator is not valid(" LINE_STRING ")");
}

graph::iterator& graph::iterator::operator *() {
    completeEdge();
    v = b;
    return *this;
}

graph::iterator& graph::iterator::operator >>(const __call& c) {
    completeEdge();
    g.all_vertices.at(v).callin = c.v;
    return *this;
}

graph::iterator& graph::iterator::operator >>(unsigned n) {
    completeEdge();
    if (!n) {
        g.all_vertices.at(v).terminal = true;
    } else if (n > MAX_FIXED_MATCH){
        throw std::logic_error("exact match size is too long(" LINE_STRING ")");
    } else {
        graph::vertex_data& vd = g.all_vertices.at(v);
        for(auto i = vd.edges.begin(); i != vd.edges.end(); ++i) {
            if (i->type != token_type2::fixed_size_match)
                continue;
            if (i->parameter != n)
                continue;
            v = e->target_vertex;
            return *this;
        }
        vd.edges.emplace_back();
        e = &vd.edges.back();
        e->type = token_type2::fixed_size_match;
        e->parameter = n;
        v = invalid_id;
    }
    return *this;
}

graph::iterator& graph::iterator::operator >>(const std::string& token) {
    completeEdge();
    graph::vertex_data& vd = g.all_vertices.at(v);
    for(auto i = vd.edges.begin(); i != vd.edges.end(); ++i) {
        if (i->type != token_type2::exact_match)
            continue;
        if (i->callback_id != token)
            continue;

        e = &(*i);
        v = e->target_vertex;
        return *this;
    }
    vd.edges.emplace_back();
    e = &vd.edges.back();
    e->type = token_type2::exact_match;
    e->callback_id = token;
    v = invalid_id;
    return *this;
}

graph::iterator& graph::iterator::operator >>(token_type tt) {
    if(tt >= token_type::max_token_type)
        throw std::logic_error("cannot match unknown token(" LINE_STRING ")");

    completeEdge();
    graph::vertex_data& vd = g.all_vertices.at(v);
    for(auto i = vd.edges.begin(); i != vd.edges.end(); ++i) {
        if (i->type != tt)
            continue;

        v = e->target_vertex;
        return *this;
    }
    vd.edges.emplace_back();
    e = &vd.edges.back();
    e->type = tt;
    v = invalid_id;
    return *this;
}

graph::iterator& graph::iterator::operator >>(const filter& f) {
    if(f.type >= token_type::max_token_type)
        throw std::logic_error("cannot match unknown token(" LINE_STRING ")");

    completeEdge();
    graph::vertex_data& vd = g.all_vertices.at(v);
    for(auto i = vd.edges.begin(); i != vd.edges.end(); ++i) {
        if (i->type != f.type)
            continue;
        if (i->callback_id != f.callback_id)
            continue;

        v = e->target_vertex;
        return *this;
    }
    vd.edges.emplace_back();
    e = &vd.edges.back();
    e->type = f.type;
    e->callback_id = f.callback_id;
    v = invalid_id;
    return *this;
}

void graph::iterator::operator >>(const graph::iterator& to) {
    assert(g.equal(to.g));
    if (e) {
        e->target_vertex = to.b;
        e = nullptr;
        v = to.b;
    } else {
        if (v == invalid_id)
            throw std::logic_error("iterator is not valid(" LINE_STRING ")");
        g.all_vertices.at(v).defaultto = to.b;
    }
}

graph::iterator::__call graph::iterator::operator ()()
{
    completeEdge();
    return __call{b};
}

graph::iterator& graph::iterator::operator ()(const std::string& cb)
{
    if (e)
        e->callback_id = cb;
    else
        g.all_vertices.at(v).callback_id = cb;
    return *this;
}

void graph::verify()
{
    unsigned maxv = all_vertices.size();

    for(auto i = root_vertices.cbegin(); i != root_vertices.cend(); ++i) {
        if (i->second >= maxv)
            throw std::runtime_error("archive contents are not valid");
    }

    for(auto i = all_vertices.begin(); i != all_vertices.end(); ++i) {
        vertex_data& vd = *i;

        if (vd.callin != invalid_id && vd.callin >= maxv)
            goto fail;
        if (vd.defaultto != invalid_id && vd.defaultto >= maxv)
            goto fail;

        for(auto j = vd.edges.begin(); j != vd.edges.end(); ++j) {
            edge_data& ed = *j;
            switch(ed.type) {
            case 0 ... token_type::max_token_type-1 :
                break;
            case token_type2::exact_match:
                if (ed.callback_id.empty())
                    goto fail;
                break;
            case token_type2::fixed_size_match:
                if (!ed.parameter || ed.parameter > MAX_FIXED_MATCH)
                    goto fail;
                break;
            default:
                goto fail;
            }
        }
    }
    return;

    fail:
    throw std::runtime_error("archive contents are not valid");
}



