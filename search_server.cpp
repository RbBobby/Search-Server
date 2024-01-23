#include "search_server.h"

SearchServer::SearchServer(const std::string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status,
    const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    storage_all_documents_.emplace_back(std::string(document));
    auto words = SplitIntoWordsNoStop(storage_all_documents_.back()); //создаем буфер для всех документов
    
    const double inv_word_count = 1.0 / words.size();
    for (const auto word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        id_word_to_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}
//-----------------FindTopDocuments ---------------------------------------------------------------------
vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

//-----------------FindTopDocuments ---------------------------------------------------------------------
vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

set<int>::const_iterator SearchServer::begin() const
{
    const auto begin = document_ids_.begin();
    return begin;
}

set<int>::const_iterator SearchServer::end() const
{
    const auto end = document_ids_.end();
    return end;
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    if (!id_word_to_freqs_.count(document_id))
    {
        static std::map<std::string_view, double> empty;
        return empty;
    }
    return id_word_to_freqs_.at(document_id);
}

tuple<vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query,
    int document_id) const {
    const auto query = ParseQuery(raw_query);

    vector<std::string_view> matched_words;
    if (std::any_of(query.minus_words.begin(), query.minus_words.end(), [&](const auto& word) {return word_to_document_freqs_.at(word).count(document_id); })) {
        return { matched_words, documents_.at(document_id).status };
    }
    for (const auto word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

tuple<vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy seq, const std::string_view raw_query, int document_id) const
{
    return MatchDocument(raw_query, document_id);
}

tuple<vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy par, const std::string_view  raw_query, int document_id) const
{
    bool flag = true;
    const auto query = ParseQuery(flag, raw_query);

    vector<std::string_view> matched_words;

    if (std::any_of(execution::par, query.minus_words.begin(), query.minus_words.end(), [&](const auto& word) {return word_to_document_freqs_.at(word).count(document_id); })) {
        return { matched_words, documents_.at(document_id).status };
    }
    matched_words.resize(query.plus_words.size());

    auto it = std::copy_if(par,
        query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(),
        [&](const auto& word) {
            if (word_to_document_freqs_.count(word) == 0) {
                return false;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {

                return true;
            }
            return false;
        });
    std::sort(execution::par, matched_words.begin(), it);
    auto it_match = std::unique(execution::par, matched_words.begin(), it);
    matched_words.erase(it_match, matched_words.end());
    return { matched_words, documents_.at(document_id).status };
}

void SearchServer::RemoveDocument(int document_id)
{
    for (const auto& [word, _] : id_word_to_freqs_.at(document_id)) {
        word_to_document_freqs_.at(word).erase(document_id);
    }

    id_word_to_freqs_.erase(document_id);

    documents_.erase(document_id);

    document_ids_.erase(document_id);
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy seq, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy par, int document_id) {
    if (id_word_to_freqs_.count(document_id))
    {
        std::vector<const std::string_view*> vector_ptr(id_word_to_freqs_.at(document_id).size()); //создаем вектор указателей на слова
        std::transform(par,
            id_word_to_freqs_.at(document_id).begin(), id_word_to_freqs_.at(document_id).end(),
            vector_ptr.begin(),
            [&](auto& lhs) {return &lhs.first; });//ПАРАЛЛЕЛЬНО заполняем вектор указателями тех слов, которые есть в документе document_id
        std::for_each(par,
            vector_ptr.begin(), vector_ptr.end(),
            [&](const auto& lhs) { word_to_document_freqs_.at(*lhs).erase(document_id); });//ПАРАЛЛЕЛЬНО удаляем из word_to_document_freqs_ эти документы, в которых есть слова из вектора vector_ptr

        id_word_to_freqs_.erase(document_id);

        documents_.erase(document_id);

        document_ids_.erase(document_id);
    }
}

bool SearchServer::IsStopWord(const string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const auto word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + string(text) + " is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(bool flag, const std::string_view text) const {
    Query result;
    for (const auto& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const {
    
    Query result = ParseQuery(true, text);
    //заменили set на vector. Теперь нужно плюс и минус отсортировать, найти неуникальные слова и убрать их

    sort(result.plus_words.begin(), result.plus_words.end());
    auto it_plus = std::unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(it_plus, result.plus_words.end());

    sort(result.minus_words.begin(), result.minus_words.end());
    auto it_minus = std::unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(it_minus, result.minus_words.end());

    return result;
}

SearchServer::Query SearchServer::ParseQuery(const execution::parallel_policy&, const std::string_view text) const {
    
    Query result;
    for (const auto& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    //заменили set на vector. Теперь нужно плюс и минус отсортировать, найти неуникальные слова и убрать их
    sort(execution::par, result.plus_words.begin(), result.plus_words.end());
    auto it_plus = std::unique(execution::par, result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(it_plus, result.plus_words.end()); 

    sort(execution::par, result.minus_words.begin(), result.minus_words.end());
    auto it_minus = std::unique(execution::par, result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(it_minus, result.minus_words.end());

    
    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}