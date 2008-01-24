#ifndef _LEDPLAY_H
#define _LEDPLAY_H

typedef enum {
	O_INT,
	O_STRING,
	O_BOOL
} OptionType;

typedef struct {
	const char *name;
	OptionType type;
	const int has_param;
	void *param;
	void *defval;
} Option;

#endif /* _LEDPLAY_H */
