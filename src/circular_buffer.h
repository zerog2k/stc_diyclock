// super simple circular buffer
// thanks to https://embedjournal.com/implementing-circular-buffer-embedded-c/

#include <stdint.h>

typedef struct {
    uint8_t * const buffer;
    int8_t head;
    int8_t tail;
    const int8_t maxLen;
} cbuf_t;

typedef enum {
    CBUF_ERROR,
    CBUF_SUCCESS
} cbuf_result_t;

int8_t cbuf_push(cbuf_t *c, uint8_t data)
{
    // next points to head  _after_ write
    int8_t next = c->head + 1;
    if (next >= c->maxLen)
        next = 0;
    
    if (next == c->tail)
        return CBUF_ERROR;      // buffer is full
    
    c->buffer[c->head] = data;
    c->head = next;
    return CBUF_SUCCESS;
}

int8_t cbuf_pop(cbuf_t *c, uint8_t *data)
{
    int8_t next;
    if (c->head == c->tail)
        return CBUF_ERROR;          // buffer empty
    
    // next points to tail _after_ read
    next = c->tail + 1;
    if (next >= c->maxLen)
        next = 0;           // loop back
    
    *data = c->buffer[c->tail];
    c->tail = next;
    return CBUF_SUCCESS;

}

void cbuf_reset(cbuf_t *c)
{
    c->head = 0;
    c->tail = 0;
}

char * cbuf_ptr(cbuf_t *c)
{
    return c->buffer;
}