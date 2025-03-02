# Overview

## Premise

Undefined behaviour is what allows the library to be fast and efficient. **Programmer errors are not handled by the functions**, so this library requires careful use. 

**Note:** This is a *serialization* library, not a parser. This means that while the functions check for the correct format, **It won't handle encoding types, special headers, or any other non-standard behavior**. Features that are not addressed indicate that they were intended to be managed either at the network level or at the client level. For more details, refer to the specific sections.

## Usage

The library is divided into two main parts: serialization and deserialization.
Both parts share the same [data structures](data-structures.md).

You can include both the serialization and deserialization headers in your project by including the main header file:

```c
#include <flashttp.h>
```

otherwise, you can selectively include the headers you need:

- [Serialization](serialization.md)
- [Deserialization](deserialization.md)