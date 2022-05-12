// Omer Faruk Bahar - 170418029 - BLM3010 - FinalÖdevi
#ifndef _SOZLUK_H_
#define _SOZLUK_H_

#include <pthread.h>

#define KEY_EXISTS 1
#define NO_KEY_EXISTS 2
#define ILLEGAL_FORMAT 3

typedef struct _dictionary_entry_t
{
	const char *key, *value;
} dictionary_entry_t;

typedef struct _dictionary_t
{
	void *root;
	pthread_mutex_t mutex;
} dictionary_t;


void dictionary_init(dictionary_t *d);
void dictionary_destroy(dictionary_t *d);
void dictionary_destroy_all(dictionary_t *d);

int dictionary_add(dictionary_t *d, const char *key, const char *value);
const char *dictionary_get(dictionary_t *d, const char *key);
int dictionary_parse(dictionary_t *d, char *key_value);
int dictionary_remove(dictionary_t *d, const char *key);

#endif
