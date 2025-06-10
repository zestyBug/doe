#ifndef SPAN_HPP_INCLUDED
#define SPAN_HPP_INCLUDED

#include <vector>
#include <array>
#include "static_array.hpp"

template<typename _Type>
class const_span;

template<typename _Type>
class span
{
	friend class const_span<_Type>;

public:
	// member types
	using element_type           = _Type;
	using value_type             = _Type;
	using size_type              = uint32_t;
	using pointer                = _Type*;
	using reference              = element_type&;
	// an iterator must have operator++ (prefix), operator!=, operator=, operator*
	using iterator               = pointer;
	using reverse_iterator       = pointer;

	constexpr span(const span&) noexcept = default;
	constexpr span():_M_ptr(nullptr),_M_count(0){}
	constexpr span(pointer data,size_type size):_M_ptr(data),_M_count(size){}
	template<typename Allocator>
	constexpr span(std::vector<_Type,Allocator>& v):_M_ptr((_Type*)v.data()),_M_count((uint32_t)v.size()){}
	template<size_t S>
	constexpr span(std::array<_Type,S>& v):_M_ptr((_Type*)v.data()),_M_count(S){}
	template<size_t S>
	constexpr span(static_array<_Type,S>& v):_M_ptr((_Type*)v.data()),_M_count((uint32_t)v.size()){}

	constexpr span& operator=(const span&) noexcept = default;
	~span() noexcept = default;



	// observers

	[[nodiscard]] constexpr
	size_type size()
	const noexcept
	{ return _M_count; }

	[[nodiscard]] constexpr
	size_type size_bytes()
	const noexcept
	{ return _M_count * sizeof(element_type); }

	[[nodiscard]] constexpr inline
	bool empty()
	const noexcept
	{ return size() == 0; }

	// element access

	constexpr
	reference front() noexcept
	{
		if(empty())
			throw std::runtime_error("front(): empty span");
		return *this->_M_ptr;
	}

	constexpr
	reference back() noexcept
	{
		if(empty())
			throw std::runtime_error("back(): empty span");
		return *(this->_M_ptr + (size() - 1));
	}

	constexpr reference operator[](size_type __idx) noexcept
	{
		return *(this->_M_ptr + __idx);
	}

	constexpr reference at(size_type __idx)
	{
		if(__idx >= size())
			throw std::out_of_range("at(): invalid index");
		return *(this->_M_ptr + __idx);
	}

	constexpr inline pointer data() noexcept
	{ return this->_M_ptr; }

	inline span<element_type>& operator++(){
		if(_M_count>0){
			_M_ptr++;
			_M_count--;
		}
		return *this;
	}

	inline void operator +=(uint32_t step) {
		if(_M_count > step){
			_M_count -= step;
			_M_ptr   += step;
		}else{
			_M_count = 0;
			_M_ptr   = nullptr;
		}
	}

	inline span<element_type> operator+(uint32_t step) {
		if(_M_count > step)
			return span<element_type>{_M_ptr+step,_M_count-step};
		else
			return span<element_type>{nullptr,0};
	}

	bool operator == (span<element_type> v) {
		if(v.size_bytes() == this->size_bytes()){
			if(0 == this->size_bytes())
				return true;
			return memcmp(this->data(),v.data(),this->size_bytes()) == 0;
		}
		return false;
	}

	// iterator support

	constexpr iterator begin() noexcept { return iterator(this->_M_ptr); }
	constexpr iterator end()   noexcept { return iterator(this->_M_ptr + this->size()); }
	constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(this->end()); }
	constexpr reverse_iterator rend()   noexcept { return reverse_iterator(this->begin()); }
private:
	pointer _M_ptr;
	size_type _M_count;
};


template<typename _Type>
class const_span
{

public:
	// member types
	using element_type           = _Type;
	using size_type              = uint32_t;
	using const_pointer          = const _Type*;
	using const_reference        = const _Type&;
	using const_iterator         = const_pointer;
	using const_reverse_iterator = const_pointer;
	

	constexpr const_span(const span<element_type>& v) noexcept :_M_ptr{v._M_ptr},_M_count{v._M_count} {} ;
	constexpr const_span(const const_span&) noexcept = default;
	constexpr const_span():_M_ptr(nullptr),_M_count(0){}
	constexpr const_span(const_pointer data,size_type size):_M_ptr(data),_M_count(size){}
	template<typename Allocator>
	constexpr const_span(const std::vector<element_type,Allocator>& v):_M_ptr((element_type*)v.data()),_M_count((uint32_t)v.size()){}
	template<size_t S>
	constexpr const_span(const std::array<element_type,S>& v):_M_ptr((element_type*)v.data()),_M_count(S){}
	template<size_t S>
	constexpr const_span(const static_array<element_type,S>& v):_M_ptr((element_type*)v.data()),_M_count((uint32_t)v.size()){}

	const_span& operator=(const span<element_type>& v) noexcept {
		this->_M_ptr = v._M_ptr;
		this->_M_count = v._M_count;
		return *this;
	};
	constexpr const_span& operator=(const const_span&) noexcept = default;
	~const_span() noexcept = default;



	// observers

	[[nodiscard]] constexpr
	size_type size()
	const noexcept
	{ return _M_count; }

	[[nodiscard]] constexpr
	size_type size_bytes()
	const noexcept
	{ return _M_count * sizeof(element_type); }

	[[nodiscard]] constexpr inline
	bool empty()
	const noexcept { 
		return size() == 0; 
	}

	// element access

	constexpr
	const_reference front()
	const noexcept {
		if(empty())
			throw std::runtime_error("front(): empty span");
		return *this->_M_ptr;
	}

	constexpr
	const_reference back()
	const noexcept {
		if(empty())
			throw std::runtime_error("back(): empty span");
		return *(this->_M_ptr + (size() - 1));
	}

	constexpr const_reference operator[](const size_type __idx) const noexcept {
		return *(this->_M_ptr + __idx);
	}
	
	constexpr const_reference at(const size_type __idx) const {
		if(__idx >= size())
			throw std::out_of_range("at(): invalid index");
		return *(this->_M_ptr + __idx);
	}

	constexpr inline const_pointer data() const noexcept
	{ return this->_M_ptr; }

	inline const_span<element_type>& operator++(){
		if(_M_count>0){
			_M_ptr++;
			_M_count--;
		}
		return *this;
	}

	inline void operator +=(uint32_t step) {
		if(_M_count > step){
			_M_count -= step;
			_M_ptr   += step;
		}else{
			_M_count = 0;
			_M_ptr   = nullptr;
		}
	}

	inline const_span<element_type> operator+(uint32_t step) {
		if(_M_count > step)
			return const_span<element_type>{_M_ptr+step,_M_count-step};
		else
			return const_span<element_type>{nullptr,0};
	}

	inline const const_span<element_type> operator+(uint32_t step) const {
		if(_M_count > step)
			return const_span<element_type>{_M_ptr+step,_M_count-step};
		else
			return const_span<element_type>{nullptr,0};
	}

	bool operator == (const const_span<element_type> v) const {
		if(v.size_bytes() == this->size_bytes()){
			if(0 == this->size_bytes())
				return true;
			return memcmp(this->data(),v.data(),this->size_bytes()) == 0;
		}
		return false;
	}

	// iterator support

	constexpr const_iterator begin() const noexcept { return const_iterator(this->_M_ptr); }
	constexpr const_iterator end()   const noexcept { return const_iterator(this->_M_ptr + this->size()); }
	constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(this->end()); }
	constexpr const_reverse_iterator rend()   const noexcept { return const_reverse_iterator(this->begin()); }
private:
	const_pointer _M_ptr;
	size_type _M_count;
};


#endif // SPAN_HPP_INCLUDED
