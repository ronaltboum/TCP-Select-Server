#include "message.h"

void print_message(message *m)
{
	printf("%.*s\n", m->length - 1, m->data);
}