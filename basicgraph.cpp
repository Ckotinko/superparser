#include "basicgraph.h"
#include <limits>
#include <cassert>
#include <stdexcept>
using namespace ckotinko::lang;

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
        throw std::logic_error("iterator is not valid");
}

graph::iterator& graph::iterator::operator >>(const std::string& token) {
    unsigned tid;
    {
        auto i = g.all_tokens.find(token);
        if (i != g.all_tokens.end())
            tid = i->second;
        else {
            tid = g.all_tokens.size();
            g.all_tokens[token] = tid;
        }
    }
    completeEdge();
    graph::vertex_data& vd = g.all_vertices.at(v);
    for(auto i = vd.edges.begin(); i != vd.edges.end(); ++i) {
        if (i->type != token_type::exact_match)
            continue;
        if (i->check_token != tid)
            continue;

        e = &(*i);
        v = e->target_vertex;
        return *this;
    }
    vd.edges.emplace_back();
    e = &vd.edges.back();
    e->type = token_type::exact_match;
    e->check_token = tid;
    v = invalid_id;
    return *this;
}

graph::iterator& graph::iterator::operator >>(token_type tt) {
    if(tt == token_type::exact_match)
        throw std::logic_error("cannot match unknown token");

    completeEdge();
    graph::vertex_data& vd = g.all_vertices.at(v);
    for(auto i = vd.edges.begin(); i != vd.edges.end(); ++i) {
        if (i->type != tt)
            continue;

        e = &(*i);
        v = e->target_vertex;
        return *this;
    }
    vd.edges.emplace_back();
    e = &vd.edges.back();
    e->type = tt;
    v = invalid_id;
    return *this;
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
            goto invalid_archive;
    }
    for(auto i = all_vertices.begin(); i != all_vertices.end(); ++i) {
        vertex_data& vd = *i;
        if (!vd.callback_id.empty())
            vd.callback = getCallback(vd.callback_id);

        if (vd.callin != invalid_id && vd.callin >= maxv)
            goto invalid_archive;
        for(auto j = vd.edges.begin(); j != vd.edges.end(); ++j) {
            edge_data& ed = *j;
            if (ed.type == token_type::exact_match) {
                bool havetoken = false;
                for(auto k = all_tokens.cbegin(); k != all_tokens.cend(); ++k) {
                    if (k->second != ed.check_token)
                        continue;
                    havetoken = true;
                    break;
                }
                if (!havetoken)
                    goto invalid_archive;
            } else if (ed.type >= token_type::max_token_type)
                goto invalid_archive;

            if (!ed.callback_id.empty())
                ed.callback = getEdgeCallback(ed.callback_id);

            if (ed.target_vertex != invalid_id && ed.target_vertex >= maxv)
                goto invalid_archive;
        }
    }
    return;

    invalid_archive:
    throw std::runtime_error("archive contents are not valid");
}
