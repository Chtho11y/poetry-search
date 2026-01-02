#pragma once

#include <vector>
#include <cstdint>
#include "restring.h"

template<typename T>
struct Matcher{
    enum Strategy{
        Single,
        Multi,
        Static,
        Bipartite,
        Dynamic,
        Regex,
        And,
        Or,
    };

    using Self = Matcher<T>;

    static const size_t INF_LENGTH = 0xfffffffu;

    std::vector<bool> cache;
    std::vector<Matcher> sub_matcher;
    size_t length_lower_bound = 0, length_upper_bound = 0;
    std::shared_ptr<T> bind_data;

    Strategy strategy;

    Matcher(Strategy strategy) : strategy(strategy){}

    static Self create_single_matcher(std::vector<bool>& cache, std::shared_ptr<T> bind_data = nullptr){
        Self matcher(Self::Single);
        matcher.cache = cache;
        matcher.length_lower_bound = 1;
        matcher.length_upper_bound = 1;
        matcher.bind_data = bind_data;
        return matcher;
    }

    static Self create_multi_matcher(std::vector<Self>& sub_matcher, std::shared_ptr<T> bind_data = nullptr, size_t length_l = 0, size_t length_u = INF_LENGTH){
        if(sub_matcher.size() != 1){
            throw std::logic_error("multi matcher should have only one sub matcher");
        }
        Self matcher(Self::Multi);
        matcher.sub_matcher = sub_matcher;
        auto l = sub_matcher[0].length_lower_bound;
        auto u = sub_matcher[0].length_upper_bound;
        matcher.length_lower_bound = l * length_l;
        matcher.length_upper_bound = std::min(u * length_u, INF_LENGTH);
        return matcher;
    }

    static Self create_seq_matcher(std::vector<Self>& sub_matcher, std::shared_ptr<T> bind_data = nullptr){
        if(sub_matcher.size() == 0){
            throw std::logic_error("seq matcher should have at least one sub matcher");
        }

        Self matcher(Self::Static);
        size_t l = 0, u = 0;
        for(auto& m : sub_matcher){
            l += m.length_lower_bound;
            u += m.length_upper_bound;
        }
        if(l != u){
            if(matcher.is_support_regex())
                matcher.strategy = Self::Regex;
            else
                matcher.strategy = Self::Dynamic;
        }
        
        matcher.sub_matcher = sub_matcher;
        matcher.length_lower_bound = l;
        matcher.length_upper_bound = u;
        return matcher;
    }

    static Self create_bipartite_matcher(std::vector<Self>& sub_matcher, std::shared_ptr<T> bind_data = nullptr){
        if(sub_matcher.size() == 0){
            throw std::logic_error("bipartite matcher should have at least one sub matcher");
        }
        for(auto& m : sub_matcher){
            if(m.strategy != Self::Single){
                throw std::logic_error("bipartite matcher should have only single matcher as sub matcher");
            }
        }
        Self matcher(Self::Bipartite);
        matcher.sub_matcher = sub_matcher;
        matcher.length_lower_bound = sub_matcher.size();
        matcher.length_upper_bound = sub_matcher.size();
        return matcher;
    }

    static Self create_logic_matcher(std::vector<Self>& sub_matcher, Strategy strategy, std::shared_ptr<T> bind_data = nullptr){
        if(sub_matcher.size() == 0){
            throw std::logic_error("logic matcher should have at least one sub matcher");
        }
        if(strategy != Matcher::And && strategy != Matcher::Or){
            throw std::logic_error("invalid logic matcher");
        }
        Self matcher(strategy);
        matcher.sub_matcher = sub_matcher;
        matcher.length_lower_bound = sub_matcher[0].length_lower_bound;
        matcher.length_upper_bound = sub_matcher[0].length_upper_bound;
        for(auto& m : sub_matcher){
            matcher.length_lower_bound = std::min(m.length_lower_bound, matcher.length_lower_bound);
            matcher.length_upper_bound = std::max(m.length_upper_bound, matcher.length_upper_bound);
        }
        return matcher;
    }

    bool is_static(){
        return length_lower_bound == length_upper_bound;
    }

    std::vector<size_t> batch_match(const std::vector<ReString>& sentences) const{
        std::vector<size_t> result;
        for(size_t i = 0; i < sentences.size(); ++i){
            if(match(sentences[i], 0, sentences[i].size())){
                result.push_back(i);
            }
        }
        return result;
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
            case Regex:
                return regex_match(str, start, end);
            case And:
                return logic_and_match(str, start, end);
            case Or:
                return logic_or_match(str, start, end);
        }
        return false;
    }

    bool single_match(const ReString& str, size_t start, size_t end) const{
        return cache[str[start]];
    }

    bool multi_match(const ReString& str, size_t start, size_t end) const{
        throw std::logic_error("multi match not implemented");
    }

    bool static_match(const ReString& str, size_t start, size_t end) const{
        if(start >= end || end - start != length_lower_bound)
            return false;
        size_t pos = start;
        for(auto& m: sub_matcher){
            size_t nxt = pos + m.length_lower_bound;
            if(!m.match(str, pos, std::min(nxt, end)))
                return false;
            pos = nxt;
        }
        return pos == end;
    }

    bool bipartite_match(const ReString& str, size_t start, size_t end) const{
        if(start >= end)
            return false;
        size_t m = end - start;
        size_t n = sub_matcher.size();
        if(m > n)
            return false;
        std::vector<std::vector<bool>> sat(m, std::vector<bool>(n, false));
        for(size_t i = 0; i < m; ++i){
            for(size_t j = 0; j < n; ++j){
                sat[i][j] = sub_matcher[j].match(str, start + i, end);
            }
        }

        std::vector<int> matchR(n, -1);
        std::vector<bool> visited(n);
        
        // DFS function to find augmenting path
        std::function<bool(int)> dfs = [&](int u) -> bool {
            for (int v = 0; v < n; ++v) {
                if (sat[u][v] && !visited[v]) {
                    visited[v] = true;
                    if (matchR[v] == -1 || dfs(matchR[v])) {
                        matchR[v] = u;
                        return true;
                    }
                }
            }
            return false;
        };
        
        // Try to match each character in string
        int result = 0;
        for (int u = 0; u < m; ++u) {
            visited.assign(n, false);
            if (dfs(u)) {
                result++;
            }
        }
        
        return result >= m;
    }

    bool regex_match(const ReString& str, size_t start, size_t end) const{
        char c = 'A';
        std::string normal_str = "";
        std::map<int16_t, char> char_map;
        for(size_t i = start; i < end; ++i){
            auto code = str[i];
            if(char_map.find(code) == char_map.end()){
                char_map[code] = c;
                c++;
            }
            normal_str += char_map[code];
        }
        auto regex = to_regex(char_map);
        if(!regex.has_value())
            return false;
        return std::regex_match(normal_str, std::regex(regex.value()));
    }

    bool dynamic_match(const ReString& str, size_t start, size_t end) const{
        throw std::logic_error("dynamic match not implemented");
    }

    bool logic_and_match(const ReString& str, size_t start, size_t end) const{
        for(auto& m : sub_matcher){
            if(!m.match(str, start, end))
                return false;
        }
        return true;
    }

    bool logic_or_match(const ReString& str, size_t start, size_t end) const{
        for(auto& m : sub_matcher){
            if(m.match(str, start, end))
                return true;
        }
        return false;
    }

    std::optional<std::string> to_regex(std::map<int16_t, char>& char_map) const {
        switch (strategy){

        case Single:{
            std::string str = "[";
            for(auto [code, ch]: char_map){
                if(cache[code]){
                    str += ch;
                }
            }
            str += "]";
            if(str == "[]")
                return std::nullopt;
            return str;
        }

        case Static: case Dynamic: case Regex:{
            std::string res = "(";
            for(auto& m : sub_matcher){
                auto val = m.to_regex(char_map);
                if(!val.has_value())
                    return std::nullopt;
                res += val.value();
            }
            res += ")";
            if(res == "()")
                return std::nullopt;
            return res;
        }

        case Multi:{
            auto res = sub_matcher[0].to_regex(char_map);
            if(!res.has_value())
                return std::nullopt;
            if(length_upper_bound >= INF_LENGTH)
                return res.value() + (length_lower_bound == 0 ? "*" : "+");
            return res.value() + "{" + std::to_string(length_lower_bound) + "," + std::to_string(length_upper_bound) + "}";
        }

        case Bipartite:{
            throw std::logic_error("bipartite match not implemented");
        }

        case And:{
            throw std::logic_error("logic and match not implemented");
            // res = "(";
            // for(auto& m : sub_matcher){
            //     auto val = m.to_regex(char_map);
            //     if(!val.has_value())
            //         return std::nullopt;
            //     res += val.value();
            // }
            // res += ")";
            // return res == "()" ? std::nullopt : res;
        }

        case Or:{
            std::string res = "(";
            bool first = true;
            for(auto& m : sub_matcher){
                auto val = m.to_regex(char_map);
                if(!val.has_value())
                    continue;
                if(!first)
                    res += "|";
                first = false;
                res += val.value();
            }
            res += ")";
            if(res == "()")
                return std::nullopt;
            return res;
        }

        default:
            return std::nullopt;
        }
    }

    bool is_support_regex() const{
        bool res = true;
        for(auto& m : sub_matcher){
            res &= m.is_support_regex();
        }
        return res && strategy != Bipartite && strategy != And;
    }

    std::string to_string(size_t indent = 0) const{
        auto indent_str = std::string(indent, ' ');
        std::string res = indent_str;
        switch (strategy){
            case Single:
                res += "SingleMatcher";
                break;
            case Multi:
                res += "MultiMatcher";
                break;
            case Static:
                res += "SeqMatcher[Static]";
                break;
            case Bipartite:
                res += "BipartiteMatcher";
                break;
            case Regex:
                res += "SeqMatcher[Regex]";
                break;
            case Dynamic:
                res += "SeqMatcher[Dynamic]";
                break;
            case And:
                res += "And";
                break;
            case Or:
                res += "Or";
                break;
            default:
                res += "Unknown";
        }
        
        if(sub_matcher.size() != 0){
            res += "(\n";
            for(auto& m : sub_matcher){
                res += m.to_string(indent + 4);
                res += "\n";
            }
            res += indent_str + ")";
        }else if(bind_data){
            res += "(" + bind_data->toString() + ")";
        }
        return res;
    }
};