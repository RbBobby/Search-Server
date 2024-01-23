#pragma once
#include <vector>
#include <mutex>
#include <map>



template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");
    struct Bucket {
        std::mutex mutex_;
        std::map<Key, Value> map_;
    };

    struct Access {
        Access(const Key& key, Bucket& bucket) : guard(bucket.mutex_), ref_to_value(bucket.map_[key]) {}
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    void erase(const Key& key) {
        std::lock_guard guard(vec_buckets_[static_cast<uint64_t>(key) % vec_buckets_.size()].mutex_);
        vec_buckets_[static_cast<uint64_t>(key) % vec_buckets_.size()].map_.erase(key);

    };

    explicit ConcurrentMap(size_t bucket_count)
        : vec_buckets_(bucket_count) {

    };

    Access operator[](const Key& key) {//get access
        auto& bucket = vec_buckets_[static_cast<uint64_t>(key) % vec_buckets_.size()];
        return { key, bucket };
    };

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> ordinary_map_;
        for (auto& bucket : vec_buckets_) {
            std::lock_guard guard(bucket.mutex_);
            ordinary_map_.insert(bucket.map_.begin(), bucket.map_.end());
        }
        return ordinary_map_;
    }

private:
    std::vector<Bucket> vec_buckets_;
};