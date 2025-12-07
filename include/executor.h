#pragma once

#include <vector>
#include <cstdint>
#include "restring.h"
#include "database.h"

struct Matcher{
    enum Strategy{
        Single,
        Multi,
        Static,
        Bipartite,
        Dynamic,
        And,
        Or,
    };

    static const size_t INF_LENGTH = 0xfffffffu;

    std::vector<bool> cache;
    std::vector<Matcher> sub_matcher;
    size_t length_lower_bound = 0, length_upper_bound = 0; 

    Strategy strategy;

    Matcher(Strategy strategy) : strategy(strategy){}

    static Matcher create_single_matcher(std::vector<bool>& cache){
        Matcher matcher(Matcher::Single);
        matcher.cache = cache;
        matcher.length_lower_bound = 1;
        matcher.length_upper_bound = 1;
        return matcher;
    }

    static Matcher create_multi_matcher(std::vector<Matcher>& sub_matcher, size_t length_l = 0, size_t length_u = INF_LENGTH){
        if(sub_matcher.size() != 1){
            throw std::logic_error("multi matcher should have only one sub matcher");
        }
        Matcher matcher(Matcher::Multi);
        matcher.sub_matcher = sub_matcher;
        auto l = sub_matcher[0].length_lower_bound;
        auto u = sub_matcher[0].length_upper_bound;
        matcher.length_lower_bound = l * length_l;
        matcher.length_upper_bound = std::min(u * length_u, INF_LENGTH);
        return matcher;
    }

    static Matcher create_seq_matcher(std::vector<Matcher>& sub_matcher){
        if(sub_matcher.size() == 0){
            throw std::logic_error("seq matcher should have at least one sub matcher");
        }

        Matcher matcher(Matcher::Static);
        size_t l = 0, u = 0;
        for(auto& m : sub_matcher){
            l += m.length_lower_bound;
            u += m.length_upper_bound;
        }
        if(l != u)
            matcher.strategy = Matcher::Dynamic;
        
        matcher.sub_matcher = sub_matcher;
        matcher.length_lower_bound = l;
        matcher.length_upper_bound = u;
        return matcher;
    }

    static Matcher create_bipartite_matcher(std::vector<Matcher>& sub_matcher){
        if(sub_matcher.size() == 0){
            throw std::logic_error("bipartite matcher should have at least one sub matcher");
        }
        for(auto& m : sub_matcher){
            if(m.strategy != Matcher::Single){
                throw std::logic_error("bipartite matcher should have only single matcher as sub matcher");
            }
        }
        Matcher matcher(Matcher::Bipartite);
        matcher.sub_matcher = sub_matcher;
        matcher.length_lower_bound = sub_matcher.size();
        matcher.length_upper_bound = sub_matcher.size();
        return matcher;
    }

    static Matcher create_logic_matcher(std::vector<Matcher>& sub_matcher, Strategy strategy){
        if(sub_matcher.size() == 0){
            throw std::logic_error("logic matcher should have at least one sub matcher");
        }
        if(strategy != Matcher::And && strategy != Matcher::Or){
            throw std::logic_error("invalid logic matcher");
        }
        Matcher matcher(strategy);
        matcher.sub_matcher = sub_matcher;
        for(auto& m : sub_matcher){
            matcher.length_lower_bound += m.length_lower_bound;
            matcher.length_upper_bound += m.length_upper_bound;
        }
        return matcher;
    }

    bool match(const ReString& str, size_t start, size_t end) const{
        switch (strategy){
            case Single:
                return single_match(str, start, end);
            case Multi:
                return multi_match(str, start, end);
            case Static:
                return static_match(str, start, end);
            case Bipartite:
                return bipartite_match(str, start, end);
            case Dynamic:
                return dynamic_match(str, start, end);
        }
        return false;
    }

    bool single_match(const ReString& str, size_t start, size_t end) const{
        return cache[str[start]];
    }

    bool multi_match(const ReString& str, size_t start, size_t end) const{
        
    }

    bool static_match(const ReString& str, size_t start, size_t end) const{
        size_t pos = start;
        for(auto& m: sub_matcher){
            if(!m.match(str, pos, end))
                return false;
            pos += m.length_lower_bound;
        }
        return true;
    }

    bool bipartite_match(const ReString& str, size_t start, size_t end) const{
        
    }

    bool dynamic_match(const ReString& str, size_t start, size_t end) const{
        
    }

    std::string to_string() const{
        
    }
};

struct QueryResult{
    size_t poetry_id;
    std::vector<size_t> match_positions;
};

struct Executor{
    enum Strategy{
        Sequential,
        Parallel,
    };

    std::vector<QueryResult> execute(Matcher* matcher, std::vector<PoetryItem>& items, Strategy strategy){
        if(strategy == Sequential){
            std::vector<QueryResult> results;
            for(auto& item : items){
                QueryResult result;
                result.poetry_id = item.id;
                for(size_t i = 0; i < item.content.size(); ++i){
                    auto& cont = item.sentences[i];
                    if(matcher->match(cont, 0, cont.size())){
                        result.match_positions.push_back(i);
                    }   
                }
                if(result.match_positions.size() != 0)
                    results.push_back(result);
            }
            return results;
        }else if(strategy == Parallel){
            throw std::logic_error("not implemented");
        }else{
            throw std::logic_error("invalid strategy");
        }
    }
};