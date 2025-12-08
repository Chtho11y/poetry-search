#pragma once

#include "cond_parser.h"
#include "database.h"

struct QueryResult{
    size_t poetry_id;
    std::vector<size_t> match_positions;
};

enum class ExecuteStrategy{
    Sequential,
    Parallel,
};

template<ExecuteStrategy strategy>
struct Executor{
    std::vector<QueryResult> execute(cond_matcher& matcher, const std::vector<PoetryItem>& items){
        static_assert(false, "Invalid execute strategy");
    }
};

template<>
struct Executor<ExecuteStrategy::Sequential>{
    std::vector<QueryResult> execute(cond_matcher& matcher, const std::vector<PoetryItem>& items){
        std::vector<QueryResult> results;
        for(auto& item : items){
            auto res = matcher.batch_match(item.sentences);
            if(!res.empty()){
                results.push_back({item.id, res});
            }
        }
        return results;
    }
};

template<>
struct Executor<ExecuteStrategy::Parallel>{
    std::vector<QueryResult> execute(cond_matcher& matcher, const std::vector<PoetryItem>& items){
        std::vector<QueryResult> results;
        
        #pragma omp parallel for
        for(int i = 0; i < items.size(); ++i){
            auto& item = items[i];
            auto res = matcher.batch_match(item.sentences);
            if(!res.empty()){
                #pragma omp critical
                {
                    results.push_back({item.id, res});
                }
            }
        }
        return results;
    }
};