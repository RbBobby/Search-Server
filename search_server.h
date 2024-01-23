#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <utility>
#include <algorithm>
#include <stdexcept>
#include <execution>
#include <cmath>
#include <deque>
#include <future>

#include "log_duration.h"
#include "document.h"
#include "concurrent_map.h"
//#include "read_input_function.h"

#include "string_processing.h"

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double EPSILON = 1e-6;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string& stop_words_text);

    explicit SearchServer(const std::string_view stop_words_text);

    void AddDocument(int document_id, const string_view document, DocumentStatus status,
        const vector<int>& ratings);

    //-----------------FindTopDocuments ---------------------------------------------------------------------
    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const std::string_view raw_query,
        DocumentPredicate document_predicate) const;

    vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;

    vector<Document> FindTopDocuments(const std::string_view raw_query) const;

   
    //-----------------FindTopDocuments execution::par-------------------------------------------------------
    template <typename Policy, typename DocumentPredicate>
    vector<Document> FindTopDocuments(const Policy& policy, const std::string_view raw_query,
        DocumentPredicate document_predicate) const;

    template <typename Policy>
    vector<Document> FindTopDocuments(const Policy& policy, const std::string_view raw_query, DocumentStatus status) const;

    template <typename Policy>
    vector<Document> FindTopDocuments(const Policy& policy, const std::string_view raw_query) const;

    int GetDocumentCount() const;

    set<int>::const_iterator begin() const;

    set<int>::const_iterator end() const;


    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    tuple<vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query,
        int document_id) const;
    tuple<vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy seq, const std::string_view raw_query,
        int document_id) const;
    tuple<vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy par, const std::string_view raw_query,
        int document_id) const;

    void RemoveDocument(int document_id);

    void RemoveDocument(std::execution::sequenced_policy seq, int document_id);

    void RemoveDocument(std::execution::parallel_policy par, int document_id);

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    std::deque<std::string> storage_all_documents_;

    const set<string, less<>> stop_words_;
    map<std::string_view, map<int, double>> word_to_document_freqs_;
    map<int, map<std::string_view, double>> id_word_to_freqs_;
    map<int, DocumentData> documents_;
    set<int> document_ids_;

    bool IsStopWord(const string_view word) const;

    static bool IsValidWord(const string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    QueryWord ParseQueryWord(const std::string_view text)const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };


    Query ParseQuery(const std::string_view text) const;

    Query ParseQuery(bool flag, const std::string_view text) const;

    Query ParseQuery(const execution::parallel_policy&, const std::string_view text) const;

    //-----------------FindAllDocuments ---------------------------------------------------------------------
    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const;

    //-----------------FindAllDocuments execution::par-------------------------------------------------------
    template <typename Policy, typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Policy& policy, const Query& query,
        DocumentPredicate document_predicate) const;
    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;


};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw invalid_argument("Some of stop words are invalid"s);
    }
}
//-----------------FindTopDocuments ---------------------------------------------------------------------
template <typename DocumentPredicate>
vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    
    return FindTopDocuments(execution::seq, raw_query, document_predicate);
}

//-----------------FindTopDocuments typename Policy-------------------------------------------------------
template <typename Policy, typename DocumentPredicate>
vector<Document> SearchServer::FindTopDocuments(const Policy& policy, const std::string_view raw_query,
    DocumentPredicate document_predicate) const {

    const auto query = ParseQuery(raw_query); //Можно распараллелить этот метод 
    auto matched_documents = FindAllDocuments(policy, query, document_predicate);


    sort(policy, matched_documents.begin(), matched_documents.end(),//Можно распараллелить
        [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}
//-----------------FindTopDocuments typename Policy-------------------------------------------------------
template <typename Policy>
vector<Document> SearchServer::FindTopDocuments(const Policy& policy, const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy,
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}
//-----------------FindTopDocuments typename Policy-------------------------------------------------------
template <typename Policy>
vector<Document> SearchServer::FindTopDocuments(const Policy& policy, const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}
//-----------------FindAllDocuments ---------------------------------------------------------------------
template <typename DocumentPredicate>
vector<Document> SearchServer::FindAllDocuments(const Query& query,
    DocumentPredicate document_predicate) const {
    return FindAllDocuments(execution::seq, query, document_predicate);
}
//-----------------FindAllDocuments typename Policy-------------------------------------------------------
template <typename Policy, typename DocumentPredicate> // шаблонная политика, чтобы можно было выбрать между par/seq
vector<Document> SearchServer::FindAllDocuments(const Policy& policy, const Query& query,
    DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(500);//используем concurrent map

    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(),
        [&](const auto& word) {
            if (!(find_if(policy,
            word_to_document_freqs_.begin(), word_to_document_freqs_.end(),
            [&word](const auto& elem) {
                    return elem.first == word;
                }) == word_to_document_freqs_.end())) { //вместо count параллельный find_if

                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                std::for_each(policy,
                    word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(),
                    [&](const auto& elem) {
                        const auto& document_data = documents_.at(elem.first);
                        if (document_predicate(elem.first, document_data.status, document_data.rating)) {
                            document_to_relevance[elem.first].ref_to_value += elem.second * inverse_document_freq;//используем concurrent map
                        }
                    });
            }
        });

    std::for_each(policy, query.minus_words.begin(), query.minus_words.end(),
        [&](const auto& word) {
            if (!(find_if(policy,
            word_to_document_freqs_.begin(), word_to_document_freqs_.end(),
            [&word](const auto& elem) {
                    return elem.first == word;
                }) == word_to_document_freqs_.end())) {//вместо count параллельный find_if
                std::for_each(policy,
                    word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(),
                    [&document_to_relevance](const auto& document_id_) {document_to_relevance.erase(document_id_.first); });
            }
        });
    vector<Document> matched_documents;

    for (const auto& [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {//используем concurrent map
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}