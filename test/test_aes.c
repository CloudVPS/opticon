#include <libopticon/datatypes.h>
#include <libopticon/util.h>
#include <libopticon/auth.h>
#include <libopticon/codec.h>
#include <libopticon/compress.h>
#include <libopticon/aes.h>
#include <assert.h>
#include <stdio.h>

int main (int argc, const char *argv[]) {
    uint8_t testbuf[4096];
    char rdstring[128];
    aeskey key = aeskey_create ();
    ioport *IO = ioport_create_buffer ((char *) testbuf, 4096);
    assert (ioport_write_u64 (IO, 0x123456789abcdef0ULL));
    assert (ioport_write_encstring (IO, "hello world."));
    
    uint8_t encbuf[4096];
    ioport *CrIO = ioport_create_buffer ((char *) encbuf, 4096);
    time_t tnow = time (NULL);
    assert (ioport_encrypt (&key, IO, CrIO, tnow, 0));
    
    ioport_close (IO);
    IO = ioport_create_buffer ((char *) testbuf, 4096);
    assert (! ioport_decrypt (&key, CrIO, IO, tnow + 3600, 0));
    ioport_close (IO);
    ioport_reset_read (CrIO);
    IO = ioport_create_buffer ((char *) testbuf, 4096);
    assert (ioport_decrypt (&key, CrIO, IO, tnow, 0));
    
    assert (ioport_read_u64 (IO) == 0x123456789abcdef0ULL);
    assert (ioport_read_encstring (IO, rdstring));
    assert (strcmp (rdstring, "hello world.") == 0);
    return 0;
}
