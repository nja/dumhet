#ifndef _search_h
#define _search_h

#include <dht/table.h>

typedef struct Search {
    DhtTable *table;
} Search;

Search *Search_Create(DhtHash *id);
void Search_Destroy(Search *search);

int Search_CopyTable(Search *search, DhtTable *source);

#endif
