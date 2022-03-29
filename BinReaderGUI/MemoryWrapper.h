#ifndef _MEMORYWRAPPER_H_
#define _MEMORYWRAPPER_H_

#include <inttypes.h>
#include <mimalloc.h>

#include <new>
#include <memory>
#include <fstream>

#include <string>
#include <list>
#include <deque>
#include <vector>
#include <unordered_map>

#include <imgui/imgui.h>

#include "Myassert.h"

void *mallocb(size_t size);
void *reallocb(void *pointer, size_t size);
void freeb(void *pointer);

void *MallocbWrapper(size_t size, void *user_data);
void FreebWrapper(void *ptr, void *user_data);

using mi_string = std::basic_string<char, std::char_traits<char>, mi_stl_allocator<char>>;
using mi_istringstream = std::basic_istringstream<char, std::char_traits<char>, mi_stl_allocator<char>>;

template<class T>
using mi_list = std::list<T, mi_stl_allocator<T>>;

template<class T>
using mi_deque = std::deque<T, mi_stl_allocator<T>>;

template<class T>
using mi_vector = std::vector<T, mi_stl_allocator<T>>;

template<class K, class V, class H = std::hash<K>>
using mi_unordered_map = std::unordered_map<K, V, H, std::equal_to<K>, mi_stl_allocator<std::pair<const K, V>>>;

//using mi_string = std::basic_string<char>;
//using mi_istringstream = std::basic_istringstream<char>;
//
//template<class T>
//using mi_list = std::list<T>;
//
//template<class T>
//using mi_deque = std::deque<T>;
//
//template<class T>
//using mi_vector = std::vector<T>;
//
//template<class K, class V, class H = std::hash<K>>
//using mi_unordered_map = std::unordered_map<K, V, H, std::equal_to<K>>;

#endif //_MEMORYWRAPPER_H_