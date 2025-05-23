#ifndef SPAN_HPP_INCLUDED
#define SPAN_HPP_INCLUDED

#include <vector>
template<typename _Type>
class span
{

public:
	// member types
	using element_type           = _Type;
	using value_type             = _Type;
	using size_type              = size_t;
	using difference_type        = ptrdiff_t;
	using pointer                = _Type*;
	using const_pointer          = const _Type*;
	using reference              = element_type&;
	using const_reference        = const element_type&;
	// an iterator must have operator++ (prefix), operator!=, operator=, operator*
	using iterator               = pointer;
	using reverse_iterator       = pointer;

	constexpr span(const span&) noexcept = default;
	constexpr span():_M_ptr(nullptr),_M_count(0){}
	constexpr span(_Type*data,size_t size):_M_ptr(data),_M_count(size){}
	constexpr span(const std::vector<_Type>& v):_M_ptr((_Type*)v.data()),_M_count(v.size()){}
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
	reference front()
	const noexcept
	{
		__glibcxx_assert(!empty());
		return *this->_M_ptr;
	}

	constexpr
	reference back()
	const noexcept
	{
		__glibcxx_assert(!empty());
		return *(this->_M_ptr + (size() - 1));
	}

	constexpr
	reference operator[](const size_type __idx)
	const noexcept
	{
		__glibcxx_assert(__idx < size());
		return *(this->_M_ptr + __idx);
	}

	constexpr
	reference at(const size_type __idx)
	const noexcept
	{
		__glibcxx_assert(__idx < size());
		return *(this->_M_ptr + __idx);
	}

	constexpr inline
	pointer data()
	const noexcept
	{ return this->_M_ptr; }

	inline span<element_type>& operator++(){
		if(_M_count>0){
			_M_ptr++;
			_M_count--;
		}
		return *this;
	}
	
	inline span<element_type> operator+(uint32_t step) const {
		if(_M_count > step)
			return span<element_type>{_M_ptr+step,_M_count-step};
		else
			return span<element_type>{nullptr,0};
	}

	bool operator == (const span<element_type> v) const {
		if(v.size_bytes() == this->size_bytes()){
			if(0 == this->size_bytes())
				return true;
			return memcmp(this->data(),v.data(),this->size_bytes()) == 0;
		}
		return false;
	}

	// iterator support

	constexpr iterator begin() const noexcept { return iterator(this->_M_ptr); }
	constexpr iterator end()   const noexcept { return iterator(this->_M_ptr + this->size()); }
	constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator(this->end()); }
	constexpr reverse_iterator rend()   const noexcept { return reverse_iterator(this->begin()); }
private:
	pointer _M_ptr;
	size_type _M_count;
};


#endif // SPAN_HPP_INCLUDED
