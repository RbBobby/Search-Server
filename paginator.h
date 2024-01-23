#pragma once
#include <iostream>

#include "search_server.h"

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end)
        :it_begin_(begin),
        it_end_(end)
    {

    }
    Iterator begin() {
        return it_begin_;
    }
    Iterator end() {
        return it_end_;
    }
    size_t size() {
        return distance(it_begin_, it_end_);
    }
private:
    Iterator it_begin_;
    Iterator it_end_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size)
    {
        auto it = begin;
        auto size_documents = distance(begin, end);
        auto count_pages = ceil(size_documents / (page_size + 0.0));
        if (page_size != 0)
        {
            for (int i = 0; i != count_pages; ++i) {

                if (distance(it, end) < page_size) {
                    pages_doc.push_back(IteratorRange(it, end));
                    break;
                }
                else {
                    pages_doc.push_back(IteratorRange(it, it + page_size));
                    advance(it, page_size);
                }
            }
        }
    }
    auto begin() const {
        return pages_doc.begin();
    }
    auto end() const {
        return pages_doc.end();
    }
    size_t size() const {
        return pages_doc.size();
    }
private:
    vector<IteratorRange<Iterator>> pages_doc;
};

template <typename Iterator>
ostream& operator<<(ostream& out, IteratorRange<Iterator> iterator_range) {
    for (auto page = iterator_range.begin(); page != iterator_range.end(); ++page) {
        out << *page;
    }
    return out;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}