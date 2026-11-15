#include "project/hash_table.h"

#include <cmath>
#include <stdexcept>
#include <utility>

HashTable::HashTable(std::size_t capacity)
    : table_(capacity), size_(0) {
    if (capacity == 0) {
        throw std::invalid_argument("Hash table capacity must be greater than zero");
    }
    for (std::size_t i = 0; i < table_.size(); ++i) {
        table_[i] = HashEntry();
    }
}

HashTable::~HashTable() = default;

std::size_t HashTable::hashFunction(const std::string& key) const {
    std::size_t hash = 2166136261u;
    for (unsigned char ch : key) {
        hash ^= ch;
        hash *= 16777619u;
    }
    return hash % table_.size();
}

std::size_t HashTable::probe(const std::string& key) const {
    std::size_t index = hashFunction(key);
    std::size_t original = index;
    std::size_t step = 1;

    while (table_[index].occupied && table_[index].key != key) {
        index = (original + step * step) % table_.size();
        ++step;
    }
    return index;
}

bool HashTable::insert(const std::string& key, int value) {
    std::size_t index = probe(key);
    if (table_[index].occupied && !table_[index].deleted) {
        return false;  // key already exists
    }

    // keep table at most half full so quadratic probing always finds a slot
    if (static_cast<double>(size_ + 1) / static_cast<double>(table_.size()) > 0.5) {
        rehash();
        index = probe(key);  // capacity change; recompute the slot
    }

    table_[index].key = key;
    table_[index].value = value;
    table_[index].occupied = true;
    table_[index].deleted = false;
    ++size_;
    return true;
}

bool HashTable::remove(const std::string& key) {
    const std::size_t index = probe(key);
    if (!table_[index].occupied || table_[index].deleted) {
        return false;
    }
    table_[index].deleted = true;  // tombstone: stays occupied so probe chains aren't broken
    --size_;
    return true;
}

bool HashTable::contains(const std::string& key) const {
    const std::size_t index = probe(key);
    return table_[index].occupied && !table_[index].deleted;
}

int HashTable::get(const std::string& key) const {
    const std::size_t index = probe(key);
    if (!table_[index].occupied || table_[index].deleted) {
        throw std::out_of_range("Key not found: " + key);
    }
    return table_[index].value;
}

bool HashTable::update(const std::string& key, int newValue) {
    const std::size_t index = probe(key);
    if (!table_[index].occupied || table_[index].deleted) {
        return false;
    }
    table_[index].value = newValue;
    return true;
}

std::size_t HashTable::size() const noexcept {
    return size_;
}

std::size_t HashTable::capacity() const noexcept {
    return table_.size();
}

bool HashTable::empty() const noexcept {
    return size_ == 0;
}

double HashTable::loadFactor() const noexcept {
    return static_cast<double>(size_) / static_cast<double>(table_.size());
}

std::vector<std::string> HashTable::keys() const {
    std::vector<std::string> result;
    for (const auto& entry : table_) {
        if (entry.occupied && !entry.deleted) {
            result.push_back(entry.key);
        }
    }
    return result;
}

std::vector<int> HashTable::values() const {
    std::vector<int> result;
    for (const auto& entry : table_) {
        if (entry.occupied && !entry.deleted) {
            result.push_back(entry.value);
        }
    }
    return result;
}

void HashTable::rehash() {
    std::vector<HashEntry> old = std::move(table_);
    table_.assign(nextPrime(old.size() * 2), HashEntry());

    // reinsert live entries only (tombstones dropped); size_ stays the same.
    for (const HashEntry& entry : old) {
        if (entry.occupied && !entry.deleted) {
            table_[probe(entry.key)] = entry;
        }
    }
}

bool HashTable::isPrime(std::size_t value) const {
    if (value < 2) {
        return false;
    }
    if (value == 2) {
        return true;
    }
    if (value % 2 == 0) {
        return false;
    }
    for (std::size_t i = 3; i * i <= value; i += 2) {
        if (value % i == 0) {
            return false;
        }
    }
    return true;
}

std::size_t HashTable::nextPrime(std::size_t value) const {
    if (value <= 2) {
        return 2;
    }
    std::size_t candidate = value % 2 == 0 ? value + 1 : value;
    while (!isPrime(candidate)) {
        candidate += 2;
    }
    return candidate;
}