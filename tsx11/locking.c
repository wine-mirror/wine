/*
 * Thread-safe X11 locking stubs
 */

static void nop(void)
{
}

void (*wine_tsx11_lock)(void) = nop;
void (*wine_tsx11_unlock)(void) = nop;
