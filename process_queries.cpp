#include "process_queries.h"


std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries)
{
   
    std::vector<Document> output_1;
    std::vector<std::vector<Document>> output(queries.size());
    std::transform(execution::par,
        queries.begin(), queries.end(),
        output.begin(),
        [&search_server](const auto query)
        {return search_server.FindTopDocuments(execution::par, query); });
    for (const auto& query : output) {
        
        output_1.insert(output_1.end(), query.begin(), query.end());
    
    }
    return output_1;
}

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries)
{
    std::vector<Document> output_1;
    std::vector<std::vector<Document>> output(queries.size());
    std::transform(execution::par,
        queries.begin(), queries.end(),
        output.begin(),
        [&search_server](const auto query)
        {return search_server.FindTopDocuments(execution::par, query); });



    return output;
}
