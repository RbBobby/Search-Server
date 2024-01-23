#pragma once
#include "search_server.h"
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate);
    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status);
    vector<Document> AddFindRequest(const string& raw_query);

    void DequeWork(const vector<Document>& queries_documents);

    int GetNoResultRequests() const;
private:
    struct QueryResult {
        bool result = false; // состояние запроса 1: запрос не нулевой 0:запрос нулевой
        int number_query = 1;// определите, что должно быть в структуре
    };
    QueryResult queries_results;
    deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    // возможно, здесь вам понадобится что-то ещё
};

template <typename DocumentPredicate>
vector<Document>  RequestQueue::AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
    vector<Document> queries_documents = search_server_.FindTopDocuments(raw_query, document_predicate);
    DequeWork(queries_documents);
    return queries_documents;

}