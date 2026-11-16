#ifndef STORAGEVALUE_HPP
#define STORAGEVALUE_HPP

template<typename T>
struct StorageValue
{
    // Позволяет менять число через const-ссылку исходного контейнера.
    mutable T data{};
};

#endif
