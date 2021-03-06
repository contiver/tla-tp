#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "symtab.h"
#include "util.h"

#define MULTIPLIER 31
#define INIT_SIZE 1009	  /* Prime size */
#define BALANCE_FACTOR 2.0
#define RESIZE_FACTOR 2

typedef struct Node Node;
struct Node{
	char *key;
	void *value;
	Node *next;
};

struct Symtab{
	Node **buckets;
	unsigned int size;
	unsigned long entries;
};

static void expandtable(Symtab *table);
static Node *findnode(Node *nodep, char *key);
static void freebucketchain(Node *nodep);
static void freebuckets(Node **buckets, int size);
static unsigned long hash(char *key, int nbuckets);
static int mustexpand(Symtab *table);

Symtab *
newsymboltable(void){
	Symtab *sp = xmalloc(sizeof(*sp));

	sp->buckets = xcalloc(INIT_SIZE, sizeof(**sp->buckets));
	sp->size    = INIT_SIZE;
	sp->entries = 0;
	return sp;
}

void
freesymboltable(Symtab *table){
	freebuckets(table->buckets, table->size);
	free(table);
}

/* lookup with optional insertion
 * See: The practice of programming, page 56
 */
void *
lookup(Symtab *table, int insert, char *key, void *value){
	unsigned long h = hash(key, table->size);
	Node *np = findnode(table->buckets[h], key);

	if(!np){
		if(insert){
			np         = xmalloc(sizeof(*np));
			np->value  = xmalloc(sizeof(np->value)); 
			np->value  = value;
			np->key    = strdup(key);
			np->next   = table->buckets[h];
			table->buckets[h] = np;
			table->entries++;
			if(mustexpand(table))
				expandtable(table);
			return np->value;
		}
		return NULL;
	}
	if(insert)
		np->value = value;
	return np->value;
}

static Node*
findnode(Node *np, char *key){
	while(np && strcmp(np->key, key))
		np = np->next;

	return np;
}

static void
freebucketchain(Node *np){
	Node *next;

	while(np){
		next = np->next;
		free(np->key);
		free(np->value);
		free(np);
		np = next;
	}
}

static unsigned long
hash(char *key, int nbuckets){
	unsigned long hashcode = 0;

	for(int i = 0; key[i] != '\0'; i++)
		hashcode = hashcode * MULTIPLIER + key[i];

	return hashcode % nbuckets;
}

static void
freebuckets(Node **buckets, int size){
	for(int i = 0; i < size; i++)
		freebucketchain(buckets[i]);
	free(buckets);
}

static int
mustexpand(Symtab *table){
	return (double)table->entries / table->size > BALANCE_FACTOR;
}

static void
expandtable(Symtab *table){
	Node **oldbuckets = table->buckets;
	const unsigned int oldsize = table->size;

	table->size *= RESIZE_FACTOR;
	table->buckets = xcalloc(table->size, sizeof(*table->buckets));
	table->entries = 0L;

	for(unsigned int i = 0; i < oldsize; i++){
		Node *np = oldbuckets[i];
		while(np){
			lookup(table, 1, np->key, np->value);
			np = np->next;
		}
	}
	freebuckets(oldbuckets, oldsize);
}
