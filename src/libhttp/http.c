#include <libopticon/ioport.h>
#include <libopticon/ioport_buffer.h>
#include <libopticon/var_parse.h>
#include <libopticon/var_dump.h>
#include <libopticon/log.h>
#include <curl/curl.h>

size_t curlwrite (char *ptr, size_t sz, size_t n, void *pport) {
    ioport *io = (ioport *) pport;
    size_t outsz = sz*n;
    if (ioport_write (io, ptr, outsz)) return outsz;
    return 0;
}

size_t curlread (void *ptr, size_t sz, size_t nm, void *pport) {
    ioport *io = (ioport *) pport;
    size_t insz = sz*nm;
    size_t avail = ioport_read_available (io);
    if (avail < insz) insz = avail;
    if (! insz) return insz;
    if (ioport_read (io, ptr, insz)) return insz;
    return 0;
}

var *http_call (const char *mth, const char *url, var *hdr, var *data,
                var *errinfo, var *resphdr) {
    CURL *curl;
    CURLcode curlres;
    struct curl_slist *chunk = NULL;
    long http_status = 0;
    
    curl = curl_easy_init();
    if (! curl) return NULL;
    
    ioport *indata = ioport_create_buffer (NULL, 4096);
    ioport *outdata = ioport_create_buffer (NULL, 4096);
    
    curl_easy_setopt (curl, CURLOPT_URL, url);
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, curlwrite);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, indata);
    
    if (strcmp (mth, "POST") == 0) {
        curl_easy_setopt (curl, CURLOPT_POST, 1L);
    }
    else if (strcmp (mth, "PUT") == 0) {
        curl_easy_setopt (curl, CURLOPT_PUT, 1L);
    }
    else if (strcmp (mth, "DELETE") == 0) {
        curl_easy_setopt (curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }
    
    if (var_get_count (data) > 0) {
        var_write (data, outdata);
        curl_easy_setopt (curl, CURLOPT_READFUNCTION, curlread);
        curl_easy_setopt (curl, CURLOPT_READDATA, outdata);
        char slen[32];
        sprintf (slen, "%lu", ioport_read_available (outdata));
        var_set_str_forkey (hdr, "Content-Type", "application/json");
        var_set_str_forkey (hdr, "Content-Length", slen);
    }
    
    if (var_get_count (hdr) > 0) {
        char tmphdr[1024];
        
        var *myhdr = hdr->value.arr.first;
        while (myhdr) {
            snprintf (tmphdr, 1023, "%s: %s", myhdr->id, var_get_str(myhdr));
            tmphdr[1023] = 0;
            chunk = curl_slist_append (chunk, tmphdr);
            myhdr = myhdr->next;
        }
        
        if (chunk) {
            curl_easy_setopt (curl, CURLOPT_HTTPHEADER, chunk);
        }
    }
    
    var *res = var_alloc();
    
    curlres = curl_easy_perform (curl);
    
    if (curlres == CURLE_OK) {
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_status);
        if (http_status == 200) {
            var_parse_json (res, ioport_get_buffer (indata));
        } else {
            if (errinfo) {
                var_parse_json (errinfo, ioport_get_buffer (indata));
                var_set_int_forkey (errinfo, "status", http_status);
            }
            var_free (res);
            res = NULL;
        }
    }
    else {
        if (errinfo) {
            var_set_str_forkey (errinfo, "error",
                                curl_easy_strerror (curlres));
        }
        log_warn ("HTTP %s %s cURL error %s", mth, url,
                  curl_easy_strerror (curlres));
        var_free (res);
        res = NULL;
    }
    
    ioport_close (indata);
    ioport_close (outdata);        
    curl_easy_cleanup (curl);
    if (chunk) curl_slist_free_all (chunk);
    return res;    
}

