#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) :search_server_(search_server) {
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    vector<Document> queries_documents = search_server_.FindTopDocuments(raw_query, status);
    DequeWork(queries_documents);
    return queries_documents;

}
vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    vector<Document> queries_documents = search_server_.FindTopDocuments(raw_query);
    DequeWork(queries_documents);
    return queries_documents;
}

void RequestQueue::DequeWork(const vector<Document>& queries_documents) {
    if (queries_documents.size() == 0 && queries_results.number_query < min_in_day_) {
        queries_results.number_query++;
        queries_results.result = false;
        requests_.push_back(queries_results);
    }
    else if (queries_documents.size() != 0 && queries_results.number_query < min_in_day_) {
        queries_results.number_query++;
        queries_results.result = true;
        requests_.push_back(queries_results);
    }
    else if (queries_documents.size() == 0) {
        requests_.pop_front();
        queries_results.number_query--;
        queries_results.result = false;
        requests_.push_back(queries_results);
    }
    else if (queries_documents.size() != 0) {
        requests_.pop_front();
        queries_results.number_query--;
        queries_results.result = true;
        requests_.push_back(queries_results);
    }
}

int RequestQueue::GetNoResultRequests() const {
    int i = 0;
    for (auto it = requests_.begin(); it != requests_.end(); ++it) {
        if ((*it).result == 0) {
            i++;
        }
    }
    return i;
}
