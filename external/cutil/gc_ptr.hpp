#ifndef _GC_PTR_HPP_
#define _GC_PTR_HPP_ 1

#include <exception>

// contain const copy of data, refrence counting CG
template<typename T>
class counter_ptr {
public:
	typedef T Type;

	counter_ptr() {
		//std::cout << "counter_ptr()\n";
		this->ref_count = nullptr;
	}

	counter_ptr(const counter_ptr<Type>& v) {
		if (v.ref_count) {
			//std::cout << "counter_ptr(const counter_ptr<Type>& v)\n";
			this->ref_count = v.ref_count;
			v.Increase();
			memcpy(&(this->data.value), &(v.data.value), sizeof(Type));
		}
	}

	counter_ptr(counter_ptr<Type>&& v) {
		this->ref_count = v.ref_count;
		if (v.ref_count) {
			//std::cout << "counter_ptr(counter_ptr<Type>&& v)\n";
			memcpy(&(this->data.value), &(v.data.value), sizeof(Type));
			v.ref_count = nullptr;
		}
	}

	counter_ptr& operator = (const counter_ptr<Type>& v) {
		//if (this != &v)
		if (this->ref_count != v.ref_count) {
			//std::cout << "counter_ptr& operator = (const counter_ptr<Type>& v)\n";
			v.Increase();
			this->Decrease();
			this->ref_count = v.ref_count;
			if(v.ref_count)
				memcpy(&(this->data.value), &(v.data.value), sizeof(Type));
		}
		return *this;
	}

	counter_ptr& operator = (counter_ptr<Type>&& v) {
		//if (this != &v)
		if (this->ref_count != v.ref_count) {
			//std::cout << "counter_ptr& operator = (counter_ptr<Type>&& v)\n";
			this->Decrease();
			this->ref_count = v.ref_count;
			if (v.ref_count)
				memcpy(&(this->data.value), &(v.data.value), sizeof(Type));
			v.ref_count = nullptr;
		}
		return *this;
	}

	~counter_ptr() {
		//std::cout << "~counter_ptr()\n";
		this->Decrease();
	}

	Type* operator ->() {
		return &operator*();
	}
	const Type* operator ->() const {
		return &operator*();
	}

	Type& operator *() {
		if (ref_count && *ref_count > 0)
			return data.value;
		else
			return *(Type*)nullptr;
	}
	const Type& operator *() const {
		if (ref_count && *ref_count > 0)
			return data.value;
		else
			return *(Type*)nullptr;
	}

	bool operator == (const Type& v) const {
		return this->ref_count == v.ref_count;
	}
	bool operator ! () const {
		return !this->ref_count;
	}
	operator bool () const {
		return this->ref_count != nullptr;
	}
protected:

	void Increase() const {
		if (this->ref_count) {
			(*this->ref_count)++;
		}
	}
	void Decrease() const {
		if (this->ref_count) {
			(*this->ref_count)--;
			if ((*this->ref_count) == 0) {
				this->data.value.~Type();
				free(ref_count);
				ref_count = nullptr;
			}
		}
	}

	mutable long* ref_count;
	union Raw {
		Type value;
		bool _;
		Raw() :_{false} {}
		~Raw() {}
	} data;

	template <class _Ty, class... _Types>
	friend counter_ptr<_Ty> make_counter(_Types&&... _Args);

};

template <class _Ty, class... _Types>
counter_ptr<_Ty> make_counter(_Types&&... _Args) {
	counter_ptr<_Ty> ret;
	new (&ret.data.value) _Ty(std::forward<_Types>(_Args)...);
	ret.ref_count = (long*)malloc(sizeof(long));
	*ret.ref_count = 1;
	return ret;
}

template<typename Type>
struct gc_val {
	long ref_count;
	Type value;
};

template<typename T>
class weak_gc_ptr;

// pointer to actual data, refrence counting CG
template<typename T>
class gc_ptr {
public:
	typedef T Type;

	gc_ptr() : data{ nullptr } {}

	// NOTE: dont make this operations templated
	// may override class operation overloading
	
	gc_ptr(const gc_ptr<Type>& v) {
		this->data = v.data;
		this->Increase();
	}

	gc_ptr(gc_ptr<Type>&& v) {
		this->data = v.data;
		v.data = nullptr;
	}


	gc_ptr& operator = (const gc_ptr<Type>& v) {
		if (this->data != v.data) {
			v.Increase();
			this->Decrease();
			this->data = v.data;
		}
		return *this;
	}


	gc_ptr& operator = (gc_ptr<Type>&& v) {
		if ((void*)this->data != (void*)v.data) {
			this->Decrease();
			this->data = v.data;
			v.data = nullptr;
		}
		return *this;
	}

	virtual ~gc_ptr() {
		this->Decrease();
	}

	template<typename To>
	gc_ptr<To> convert() {
		gc_ptr<To> ret;
		this->Increase();
		this->data = (gc_val<To>*)(void*)(this->data);
		return ret;
	}

	Type* get() {
		if (this->data)
			return &(this->data->value);
		else
			return nullptr;
	}
	const Type* get() const {
		if (this->data)
			return &(this->data->value);
		else
			return nullptr;
	}
	void clear() {
		this->Decrease();
		this->data = nullptr;
	}

	Type* operator ->() const {
		if (this->data)
			return &this->data->value;
		else
			throw std::exception();
	}
	Type& operator *() const {
		if (this->data)
			return this->data->value;
		else
			return *(Type*)nullptr;
	}

	template<typename PolyType>
	bool operator == (const gc_ptr<PolyType>& v) const {
		return this->data == v.data;
	}
	bool operator == (const Type& v) const {
		return this->data == &v;
	}
	bool operator ! () const {
		return !this->data;
	}
	operator bool() const {
		return this->data != nullptr;
	}
protected:

	void Increase() const {
		if (this->data)
			this->data->ref_count++;
	}
	void Decrease() const {
		if (this->data) {
			this->data->ref_count--;
			if (this->data->ref_count == 0) {
				this->data->value.~Type();
				free(this->data);
				//this->data = nullptr;
			}
		}
	}
	gc_val<Type> *data;

	template <class _Ty, class... _Types>
	friend gc_ptr<_Ty> make_gc(_Types&&... _Args);

	template<typename U>
	friend class gc_ptr;
	template<typename S>
	friend class weak_gc_ptr;

};

template<typename T>
class weak_gc_ptr {
public:
	typedef T Type;

	// tries to find origin,
	weak_gc_ptr(Type* pointer) {
		const gc_val<Type>* ptr = nullptr;
		intptr_t offset = (intptr_t)&(ptr->value) - (intptr_t)ptr;
		this->data = (gc_val<Type>*)((intptr_t)pointer - offset);
	}

	weak_gc_ptr() : data{ nullptr } {}

	template<typename PolyType>
	weak_gc_ptr(const gc_ptr<PolyType>& v) {
		this->data = v.data;
	}
	template<typename PolyType>
	weak_gc_ptr(const weak_gc_ptr<PolyType>& v) {
		this->data = v.data;
	}

	template<typename PolyType>
	weak_gc_ptr& operator = (const gc_ptr<PolyType>& v) {
		this->data = v.data;
		return *this;
	}
	template<typename PolyType>
	weak_gc_ptr& operator = (const weak_gc_ptr<PolyType>& v) {
		this->data = v.data;
		return *this;
	}

	virtual ~weak_gc_ptr() {}

	Type* get() {
		if (this->data)
			return &(this->data->value);
		else
			return nullptr;
	}
	const Type* get() const {
		if (this->data)
			return &(this->data->value);
		else
			return nullptr;
	}

	gc_ptr<Type> share() {
		gc_ptr<Type> ret;
		ret.data = this->data;
		ret.Increase();
		return ret;
	}

	Type& operator ->() const {
		return operator*();
	}
	Type& operator *() const {
		if (this->data)
			return this->data->value;
		else
			return *(Type*)nullptr;
	}

	template<typename PolyType>
	bool operator == (const weak_gc_ptr<PolyType>& v) const {
		return this->data == v.data;
	}
	template<typename PolyType>
	bool operator == (const gc_ptr<PolyType>& v) const {
		return this->data == v.data;
	}
	bool operator == (const Type& v) const {
		return this->data == v.data;
	}
	bool operator ! () const {
		return !this->data;
	}
	operator bool() const {
		return this->data != nullptr;
	}
protected:
	gc_val<Type>* data;
};


template <class _Ty, class... _Types>
gc_ptr<_Ty> make_gc(_Types&&... _Args) {
	gc_ptr<_Ty> ret;
	ret.data = (gc_val<_Ty>*) malloc( sizeof(gc_val<_Ty>) );
	new (&ret.data->value) _Ty(std::forward<_Types>(_Args)...);
	     ret.data->ref_count = 1;
	return ret;
}

// C++ complicated, inherited, custom function, refrence counting CG
class object_ptr {
public:
	object_ptr() {
		puts("object_ptr::object_ptr()");
		this->__refrence_count = (int32_t*)malloc(sizeof(int32_t));
		*this->__refrence_count = 1;
		__alloc();
	}
	object_ptr(const object_ptr& obj) {
		puts("object_ptr::object_ptr(const object_ptr&)");
		this->__refrence_count = obj.__refrence_count;
		(*this->__refrence_count)++;
	}
	object_ptr(object_ptr&& obj) {
		puts("object_ptr::object_ptr(object_ptr&&)");
		this->__refrence_count = obj.__refrence_count;
		obj.__refrence_count = nullptr;
	}
	object_ptr& operator = (const object_ptr& obj) {
		puts("object_ptr::operator = (const object_ptr&)");
		if (this != &obj) {
			this->__refrence_count = obj.__refrence_count;
			(*this->__refrence_count)++;
		}
		return *this;
	}
	object_ptr& operator = (object_ptr&& obj) {
		puts("object_ptr::operator = (object_ptr&&)");
		if (this != &obj) {
			this->__refrence_count = obj.__refrence_count;
			obj.__refrence_count = nullptr;
		}
		return *this;
	}
	virtual ~object_ptr() {
		puts("object_ptr::~object_ptr()");
		if (this->__refrence_count) {
			(*this->__refrence_count)--;
			if ((*this->__refrence_count) == 0) {
				__dealloc();
				free(this->__refrence_count);
				this->__refrence_count = nullptr;
			}
		}
	}
protected:
	//called before all of contructors
	virtual void __alloc() {
		puts("object_ptr::__alloc()");
	}
	//called after all of destructors
	virtual void __dealloc() {
		puts("object_ptr::__dealloc()");
	}
private:
	int32_t* __refrence_count;
};

#endif