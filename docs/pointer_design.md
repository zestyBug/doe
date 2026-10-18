### Pointers
- Doesn't hold up any standard and its used for exceptional unsafe codes.
- May contain ownership, may be null, may be an array, So read the code and documentation.
- Allocators must return in pointer.

### Reference
- It can't be null.
- It's a temporary borrowed address. Can't hold any ownership at all.
- Can't be an array.
- Obtaining and storing it address anywhere else is unsafe.
- A struct must not have any reference members.
- The compiler may inline it.
- Returning refrence suggests returning value cant be null without throwing an exception.

### const
- use this keyword to make sure lended address remains unmodified.

### Span/View
- Its equal to reference + array size
- Can be constant
- Can be zero sized

### Smart pointers
- The intended way to hold ownership