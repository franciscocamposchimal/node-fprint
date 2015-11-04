#include <nan.h>
#include <libfprint/fprint.h>

extern int initalized;

#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

unsigned int fromFPDev(struct fp_dev *dev);
struct fp_dev* toFPDev(unsigned int value);