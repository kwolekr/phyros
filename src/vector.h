/////////// Compile-time configuration ////////////
#define VECTOR_DEFAULT_SIZE 4
//#define VECTOR_TRIM_ARRAYS
///////////////////////////////////////////////////

LPVECTOR VectorInit(unsigned int size);
LPVECTOR VectorAdd(LPVECTOR vector, void *item);
void VectorRemove(LPVECTOR vector, void *item);
void VectorClear(LPVECTOR vector);
void VectorDelete(LPVECTOR vector);

