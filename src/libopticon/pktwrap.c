#include <libopticon/pktwrap.h>
#include <libopticon/compress.h>
#include <libopticon/aes.h>
#include <netinet/in.h>
#include <fcntl.h>

uint32_t SERIAL = 0;

/** Generate a 32-bit network identifier out of an IPv4 or IPv6
  * address. In the case of v4, this will result in a straight
  * 32bits cast of the address. In the case of v6, the 4 32-bit
  * words that make up the address are XORed together.
  * \param stor The address storage.
  */
uint32_t gen_networkid (struct sockaddr_storage *stor) {
    if (stor->ss_family == AF_INET) {
        struct sockaddr_in *in = (struct sockaddr_in *) stor;
        return (uint32_t) &in->sin_addr;
    }
    else if (stor->ss_family == AF_INET6) {
        struct sockaddr_in6 *in = (struct sockaddr_in6 *) stor;
        uint32_t *addr = (uint32_t *) &in->sin6_addr;
        return addr[0] ^ addr[1] ^ addr[2] ^ addr[3];
    }
    return 0;
}

/** Generate a random sessionid. Clients are typically expected to
  * reuse the same session-id, but switch session keys at a
  * regular interval.
  */
uint32_t gen_sessionid (void) {
    uint32_t res;
    int fdevr = open ("/dev/random",O_RDONLY);
    read (fdevr, &res, sizeof (uint32_t));
    close (fdevr);
    return res;
}

/** Generate a packet serial number. The easiest way to guarantee
  * that serials don't overlap on restart of a client, the first
  * serial number generated is the current time. Subsequent
  * numbers generated just increase this by one. This implies
  * that sending packets at a constant rate of more than 1/s defies
  * the mechanism (which is fine since we're planning on sending
  * at much larger intervals).
  */
uint32_t gen_serial (void) {
    if (! SERIAL) SERIAL = time (NULL);
    else SERIAL++;
    return SERIAL;
}

/** Wraps up packet-encoded metering data in a proper meterdata packet
  * bound to an active session.
  * \param sessid The session-id to encode
  * \param serial The serial number to encode
  * \param key The AES session key to use
  * \param input The packet-encoded metering data
  * \return A buffer ioport with data, ready to be sent into space, like
  *         a monkey. Or NULL if something went wrong.
  */
ioport *ioport_wrap_meterdata (uint32_t sessid, uint32_t serial,
                               aeskey key, ioport *input) {
    ioport *gzipped = ioport_create_buffer (NULL, 2048);
    ioport *res = ioport_create_buffer (NULL, 2048);
    time_t tnow = time (NULL);
    
    int success = 0;
    
    /** Write meterdata header */
    if (ioport_write (res, "o6m1", 4) &&
        ioport_write_u32 (res, sessid) &&
        ioport_write_u32 (res, serial)) {
        if (compress_data (input, gzipped)) {
            if (ioport_encrypt (&key, gzipped, res, tnow, serial)) {
                success = 1;
            }
        }
    }
    
    ioport_close (gzipped);
    
    if (! success) {
        ioport_close (res);
        return NULL;
    }
    
    return res;
}

/** Wraps up and encrypts an authentication packet.
  * \param auth The authentication data to use.
  * \param serial The serial number to inject.
  * \return A buffer ioport with the packet data.
  */
ioport *ioport_wrap_authdata (authinfo *auth, uint32_t serial) {
    ioport *payload = ioport_create_buffer (NULL, 2048);
    ioport *res = ioport_create_buffer (NULL, 2048);
    time_t tnow = time (NULL);
    
    int success = 0;
    if (ioport_write (res, "o6a1", 4) &&
        ioport_write_uuid (res, auth->tenantid) &&
        ioport_write_u32 (res, serial) &&
        ioport_write_u32 (payload, auth->sessionid) &&
        ioport_write_uuid (payload, auth->hostid) &&
        ioport_write (payload, (const char *) &auth->sessionkey,
                      sizeof(aeskey))) {
        if (ioport_encrypt (&auth->tenantkey, payload, res, tnow, serial)) {
            success = 1;
        }
    }
    
    ioport_close (payload);
    
    if (! success) {
        ioport_close (res);
        return NULL;
    }
    
    return res;
}

/** Attempt to decrypt and unwrap a received meterdata packet.
  * \param networkid The networkid for the sending host
  * \param in An ioport containing all the juicy packet data.
  * \param resolve Function to call to look up the key for the
  *                session claimed by the packet envelope. Its 3
  *                arguments are: the networkid, the claimed
  *                sessionid and the packet serial#.
  * \return A buffer ioport for the pktcodec grinder, or NULL.
  */
ioport *ioport_unwrap_meterdata (uint32_t networkid, ioport *in,
                                 resolve_sessionkey_f resolve,
                                 void **sessiondata) {
    ioport *decrypted = ioport_create_buffer (NULL, 2048);
    ioport *res = ioport_create_buffer (NULL, 2048);
    char packettype[5];
    int success = 0;
    uint32_t serial;
    uint32_t sessid;
    aeskey *k;
    time_t tnow = time (NULL);
    
    if (ioport_read (in, packettype, 4)) {
        packettype[4] = 0;
        if (strcmp (packettype, "o6m1") == 0) {
            sessid = ioport_read_u32 (in);
            serial = ioport_read_u32 (in);
            k = resolve (networkid, sessid, serial, sessiondata);
            if (k && ioport_decrypt (k, in, decrypted, tnow, serial)) {
                if (decompress_data (decrypted, res)) {
                    success = 1;
                }
            }
        }
    }
    
    ioport_close (decrypted);
    
    if (! success) {
        ioport_close (res);
        return NULL;
    }
    
    return res;
}

/** Attempt to decrypt and unwrap a received authentication packet.
  * \param in An ioport containing the packet data.
  * \param resolve A function to call to find the decryption key
  *                for a claimed tenant.
  * \return An authinfo structure with all the details necessary
  *         to create a session, or NULL if something failed.
  */
authinfo *ioport_unwrap_authdata (ioport *in, resolve_tenantkey_f resolve) {
    authinfo *res = (authinfo *) malloc (sizeof (authinfo));
    ioport *decrypted = ioport_create_buffer (NULL, 2048);
    char packettype[5];
    int success = 0;
    uint32_t serial;
    aeskey *k;
    time_t tnow = time (NULL);

    if (ioport_read (in, packettype, 4)) {
        packettype[4] = 0;
        if (strcmp (packettype, "o6a1") == 0) {
            res->tenantid = ioport_read_uuid (in);
            serial = ioport_read_u32 (in);
            k = resolve (res->tenantid, serial);
            if (k && ioport_decrypt (k, in, decrypted, tnow, serial)) {
                res->tenantkey = *k;
                res->sessionid = ioport_read_u32 (decrypted);
                res->hostid = ioport_read_uuid (decrypted);
                if (ioport_read (decrypted, (char *) &res->sessionkey,
                                 sizeof(aeskey))) {
                    success = 1;
                }
                res->serial = serial;
            }
        }
    }
    
    ioport_close (decrypted);
    if (! success) {
        free (res);
        return NULL;
    }
    return res;
}
