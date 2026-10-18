#ifndef SPAN_HPP
#define SPAN_HPP

#include <vector>
#include <array>
#include "static_array.hpp"

template<typename _Type>
class span
{
public:
	// member types
    using value_type             = std::remove_cv_t<_Type>;
	using size_type              = uint32_t;
	using const_pointer          = const _Type*;
	using pointer                = _Type*;
	using reference              = _Type&;
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

	inline constexpr operator span<const value_type> () const {
		return span<const value_type>{(const value_type*)this->_M_ptr,this->_M_count};
	}


	// observers

	[[nodiscard]] constexpr
	size_type size()
	const noexcept
	{ return _M_count; }

	[[nodiscard]] constexpr
	size_type size_bytes()
	const noexcept
	{ return _M_count * sizeof(_Type); }

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
	constexpr inline const_pointer data() const noexcept
	{ return this->_M_ptr; }

	inline span<_Type>& operator++(){
		if(_M_count>0){
			_M_ptr++;
			_M_count--;
		}
		return *this;
	}

	inline void operator +=(size_type step) {
		if(_M_count > step){
			_M_count -= step;
			_M_ptr   += step;
		}else{
			_M_count = 0;
			_M_ptr   = nullptr;
		}
	}

	inline span<_Type> operator+(size_type step) const {
		if(_M_count > step)
			return span<_Type>{_M_ptr+step,_M_count-step};
		else
			return span<_Type>{nullptr,0};
	}

	bool operator == (const span<_Type> &v) const {
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

#endif // SPAN_HPP
