#include <datatypes.h>
#include <util.h>
#include <assert.h>
#include <stdio.h>
#include <auth.h>
#include <codec.h>
#include <compress.h>
#include <aes.h>

int main (int argc, const char *argv[]) {
    uint8_t testbuf[4096];
    char rdstring[128];
    ioport *IO = ioport_create_buffer ((char *) testbuf, 4096);
    assert (ioport_write_u64 (IO, 0x123456789abcdef0ULL));
    assert (ioport_write_bits (IO, 13, 4));
    assert (ioport_write_bits (IO, 42, 6));
    assert (ioport_write_bits (IO, 5, 3));
    assert (ioport_write_bits (IO, 69, 7));
    assert (ioport_flush_bits (IO));
    assert (ioport_write_encstring (IO, "hello world."));
    assert (ioport_write_encint (IO, 1337ULL));
    assert (ioport_write_encint (IO, 78154004ULL));
    assert (ioport_read_u64 (IO) == 0x123456789abcdef0ULL);
    assert (ioport_read_bits (IO, 4) == 13);
    assert (ioport_read_bits (IO, 6) == 42);
    assert (ioport_read_bits (IO, 3) == 5);
    assert (ioport_read_bits (IO, 7) == 69);
    assert (ioport_read_encstring (IO, rdstring));
    assert (strcmp (rdstring, "hello world.") == 0);
    assert (ioport_read_encint (IO) == 1337ULL);
    assert (ioport_read_encint (IO) == 78154004ULL);
    return 0;
}
