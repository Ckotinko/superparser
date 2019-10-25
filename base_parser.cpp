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
    e->callback_id = token;
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
graph::iterator& graph::iterator::operator >>(const filter& f) {
    if(f.type == token_type::exact_match)
        throw std::logic_error("cannot match unknown token");

    completeEdge();
    graph::vertex_data& vd = g.all_vertices.at(v);
    for(auto i = vd.edges.begin(); i != vd.edges.end(); ++i) {
        if (i->type != f.type)
            continue;
        if (i->callback_id != f.callback_id)
            continue;

        e = &(*i);
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
    if (to.v == invalid_id || to.e) {
        throw std::logic_error("destination iterator is not a valid target");
    }
    if (e && e->target_vertex == invalid_id) {
        e->target_vertex = to.v;
        e = nullptr;
        v = to.v;
    } else if (!e && v != invalid_id) {
        g.all_vertices.at(v).defaultto = to.v;
    } else
        throw std::logic_error("cannot build a edge between two vertices");
}
graph::iterator::call graph::iterator::operator ()()
{
    if (e || v == invalid_id)
        throw std::logic_error("() applicable only vertices");
    return call{v};
}
graph::iterator& graph::iterator::operator ()(const std::string& cb)
{
    if (e)
        e->callback_id = cb;
    else
        g.all_vertices.at(v).callback_id = cb;
    return *this;
}
void graph::iterator::end() {
    completeEdge();
    g.all_vertices.at(v).terminal = true;
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
        if (vd.defaultto != invalid_id && vd.defaultto >= maxv)
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
static std::string htmlify(const std::string& text) {
    std::string copy;
    unsigned n = 0, e = text.size();
    do {
        auto x = text.find_first_of("<>& \t\"", n);
        if (x == std::string::npos) {
            copy.append(text, n, x);
            break;
        }
        if (x > n)
            copy.append(text, n, x-n);
        switch(text[x]) {
        case '<':
            copy.append("&lt;"); break;
        case '>':
            copy.append("&gt;"); break;
        case '&':
            copy.append("&amp;"); break;
        case ' ':
            copy.append("&nbsp;"); break;
        case '\t':
            copy.append("&nsbp;&nsbp;&nsbp;&nsbp;"); break;
        case '\"':
            copy.append("&quot;"); break;
        default:
            copy.append(1,'?');
        }
        n = x+1;
    } while (n < e);
    return copy;
}
void graph::dump(std::ostream &os)
{
    std::unordered_map<unsigned, std::string> rmap;
    for(auto i = all_tokens.cbegin(); i != all_tokens.cend(); ++i)
        rmap[i->second] = i->first;

    os<<R"(((
<html>
<head>
   <meta encoding=\"UTF-8\">
   <style>
   h3 { display: block; }
   .wide { width: 100%; }
   .call { background-color: #fdf; }
   .root { background-color: #dff; }
   .return { background-color: #f33; }
   </style>
   <script>
   function hs(id) {
      let e = document.getElementById(id);
      if (e.style.display == "none")
        e.style.display = "block";
      else
        e.style.display = "none";
   }
   </script>
</head>
<body>)))";
    int icnt = 0;
    for(auto i = root_vertices.cbegin(); i != root_vertices.cend(); ++i, ++icnt) {
        os<<"<div class='wide'><h3 onclick=\"hs('blk)))"<<icnt<<"';\">"
          <<htmlify(i->first)<<"</h3>";
        os<<"<div style='display:none;' id='blk"<<icnt<<"'>";



        os<<"</div></div>";
    }
    os<<"</body></html>";
}
