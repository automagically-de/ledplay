#define _BSD_SOURCE 1
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "ledplay.h"

static char *o_file;
static int o_width, o_height, o_fps, o_help, o_demo;

static Option options[] = {
	{ "width", O_INT, 1, &o_width, (void *)4 },
	{ "w", O_INT, 1, &o_width, (void *)4 },
	{ "height", O_INT, 1, &o_height, (void *)3 },
	{ "h", O_INT, 1, &o_height, (void *)3 },
	{ "fps", O_INT, 1, &o_fps, (void *)10 },
	{ "f", O_INT, 1, &o_fps, (void *)10 },
	{ "input", O_STRING, 1, &o_file, NULL },
	{ "i", O_STRING, 1, &o_file, NULL },
	{ "help", O_BOOL, 0, &o_help, (void *)0 },
	{ "demo", O_BOOL, 0, &o_demo, (void *)0 },
	{ "d", O_BOOL, 0, &o_demo, (void *)0 }
};

int parse_options(int argc, char *argv[]);
uint8_t **load_file(const char *filename, int width, int height);
void cleanup(uint8_t **input);
uint32_t convert_to_int(uint8_t *frame);
int show_frame(uint8_t *frame, int fd);
int play(uint8_t **input);
void help(void);

int main(int argc, char *argv[])
{
	uint8_t **input;

	if(!parse_options(argc, argv))
		return EXIT_FAILURE;

	if(o_help)
	{
		help();
		return EXIT_FAILURE;
	}

	if(o_file == NULL)
	{
		fprintf(stderr, "%s: no input file given\n", argv[0]);
		help();
		return EXIT_FAILURE;
	}

	input = load_file(o_file, o_width, o_height);

	play(input);

	cleanup(input);

	return EXIT_SUCCESS;
}

int parse_options(int argc, char *argv[])
{
	int i, j, noptions;

	noptions = sizeof(options) / sizeof(Option);

	/* initialize option values */
	for(i = 0; i < noptions; i ++)
	{
		switch(options[i].type)
		{
			case O_STRING:
				*((char **)(options[i].param)) = (char *)options[i].defval;
				break;
			case O_INT:
			case O_BOOL:
				*((int *)(options[i].param)) = (int)options[i].defval;
				break;
		}
	}

	j = 1;
	while(j < argc)
	{
		/* check if it is an option */
		if(argv[j][0] != '-')
		{
			fprintf(stderr,
				"%s: non-option detected where option is expected: %s\n",
				argv[0], argv[j]);
			return 0;
		}

		/* try to find option in array */
		for(i = 0; i <= noptions; i ++)
		{
			if(i == noptions)
			{
				fprintf(stderr, "%s: option %s is unknown\n",
					argv[0], argv[j]);
				return 0;
			}

			if(strcmp(argv[j] + 1, options[i].name) == 0)
				break;
		}

		if(options[i].has_param)
		{
			j ++;
			if(j >= argc)
			{
				fprintf(stderr, "%s: option %s is missing a parameter\n",
					argv[0], argv[j - 1]);
				help();
				return 0;
			}
		}

		switch(options[i].type)
		{
			case O_STRING:
				*((char **)(options[i].param)) =
					calloc(strlen(argv[j]) + 1, sizeof(char));
				strcpy(*((char **)(options[i].param)), argv[j]);
				break;
			case O_INT:
				*((int *)(options[i].param)) = atoi(argv[j]);
				break;
			case O_BOOL:
				*((int *)(options[i].param)) = 1;
				break;
		}

		j ++;
	}

	return 1;
}

uint8_t **load_file(const char *filename, int width, int height)
{
	uint8_t **input = NULL;
	int nframes = 0, i, j, line = 0;
	char buf[64];
	FILE *f;

	f = fopen(filename, "r");
	do
	{
		input = (uint8_t **)realloc(input, (nframes + 1) * sizeof(uint8_t *));
		input[nframes] = (uint8_t *)calloc(width * height, sizeof(uint8_t));
		for(i = height - 1; i >= 0; i --)
		{
			memset(buf, '\0', 64);
			fgets(buf, 63, f);
			line ++;
			for(j = 0; j < width; j ++)
			{
				if(buf[j] == 'x')
					input[nframes][i * width + j] = 1;
				else if(buf[j] == '.')
					input[nframes][i * width + j] = 0;
				else
					fprintf(stderr, "line %d:%d: unknown character '%c'\n",
						line, j, buf[j]);
			}
		}
		/* empty line */
		fgets(buf, 63, f);
		line ++;
		nframes ++;
	} while(!feof(f));

	fclose(f);

	fprintf(stderr, "D: %d frames loaded from %s\n", nframes, filename);

	/* terminate array with NULL */
	input = realloc(input, (nframes + 1) * sizeof(uint8_t *));
	input[nframes] = NULL;

	return input;
}

void cleanup(uint8_t **input)
{
	uint8_t **p = input;

	while(*p != NULL)
	{
		free(*p);
		p ++;
	}

	free(input);
}

uint32_t convert_to_int(uint8_t *frame)
{
	uint32_t res = 0;
	int i;
	static uint8_t c8[] = { 0, 4, 8, 12, 16, 20, 24, 28 };
	static uint8_t c12[] = { /* D1 - D7 */ 2, 5, 8, 11, 14, 17, 20, 23,
		30, 28, 26, /* STROBE */ 0 };

	if((o_width * o_height) == 8)
	{
		for(i = 0; i < 8; i ++)
			if(frame[i])
				res |= (1 << c8[i]);
	}
	else if((o_width * o_height) == 12)
	{
		for(i = 0; i < 12; i ++)
			if(frame[i])
				res |= (1 << c12[i]);
	}
	else
		for(i = 0; i < 32; i ++)
			if(frame[i])
				res |= (1 << i);

	return res;
}

int show_frame(uint8_t *frame, int fd)
{
	uint32_t u32;

	u32 = convert_to_int(frame);
	write(fd, &u32, sizeof(u32));

	return 1;
}

int show_frame_demo(uint8_t *frame)
{
	int i, j;

	/* clear screen */
	printf("\x1b[2J\x1b[2;1H");

	/* show "LEDs" */
	for(i = 0; i < o_height; i ++)
	{
		printf(" ");
		for(j = 0; j < o_width; j ++)
		{
			printf("%s", frame[i * o_width + j] ?
				"\x1b[1;32m(*)\x1b[0m" : "( )");
		}
		printf("\n");
	}
	printf("\n");

	return 1;
}

int play(uint8_t **input)
{
	int sval, fd = 1;
	uint8_t **p = input;

	sval = 1000000 / o_fps;

	if(*p == NULL)
	{
		fprintf(stderr, "play(): no frames in input\n");
		return 0;
	}

	if(!o_demo)
	{
		fd = open("/dev/led", O_WRONLY);
		if(fd < 0)
		{
			fprintf(stderr, "failed to open /dev/led (%d: %s)\n",
				errno, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	while(1)
	{
		if(*p == NULL)
			p = input;

		if(o_demo)
			show_frame_demo(*p);
		else
			show_frame(*p, fd);
		p ++;
		usleep(sval);
	}

	if(!o_demo)
		close(fd);

	return 1;
}

void help(void)
{
	fprintf(stderr,
		"ledplay options (+ short options & defaults):\n"
		"  -input <filename>    (-i)  load input file from this file (NULL)\n"
		"  -width <width>       (-w)  width of input and output (4)\n"
		"  -height <height>     (-h)  height of input and output (3)\n"
		"  -fps <fps>           (-f)  frames per second (10)\n"
		);
}
