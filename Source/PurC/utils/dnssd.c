/**
 * @file dnssd.c
 * @author Vincent Wei
 * @date 2023/10/24
 * @brief The helpers for service discovery based on mDNS Reponsder.
 *
 * Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "purc/purc-helpers.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_TXT_RECORD_SIZE 8900
#define MAX_DOMAIN_NAME     1009
#define HOST_NAME_SUFFIX    ".local."

#define HexVal(X) ( ((X) >= '0' && (X) <= '9') ? ((X) - '0'     ) :  \
                    ((X) >= 'A' && (X) <= 'F') ? ((X) - 'A' + 10) :  \
                    ((X) >= 'a' && (X) <= 'f') ? ((X) - 'a' + 10) : 0)

#define HexPair(P) ((HexVal((P)[0]) << 4) | HexVal((P)[1]))

char *purc_get_local_hostname(char *hostname)
{
    struct addrinfo hints, *info, *p;
    int ret;

    if (gethostname(hostname, PURC_MAX_LEN_HOSTNAME)) {
        hostname[0] = 0;
        return NULL;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    if ((ret = getaddrinfo(hostname, "http", &hints, &info)) != 0) {
        hostname[0] = 0;
        return NULL;
    }

    hostname[0] = 0;
    for (p = info; p != NULL; p = p->ai_next) {
        strcpy(hostname, p->ai_canonname);
        strcat(hostname, HOST_NAME_SUFFIX);
    }

    freeaddrinfo(info);
    return hostname;
}

char *purc_get_local_hostname_alloc(void)
{
    char *buf = calloc(1, PURC_MAX_LEN_HOSTNAME + 1);
    if (buf)
        return purc_get_local_hostname(buf);

    return NULL;
}

#if PCA_ENABLE_DNSSD

#include <dns_sd.h>

typedef union {
    unsigned char b[2];
    unsigned short NotAnInteger;
} Opaque16;

struct purc_dnssd_conn {
    DNSServiceRef                   shared_ref;
    dnssd_on_register_reply         register_reply_cb;
    dnssd_on_service_discovered     service_discovered_cb;
    void                           *ctxt;
};

struct purc_dnssd_conn *purc_dnssd_connect(
        dnssd_on_register_reply         register_reply_cb,
        dnssd_on_service_discovered     service_discovered_cb,
        void *context)
{
    struct purc_dnssd_conn *dnssd = calloc(1, sizeof(struct purc_dnssd_conn));

    if (dnssd) {
        DNSServiceErrorType error;
        error = DNSServiceCreateConnection(&dnssd->shared_ref);
        if (error) {
            free(dnssd);
            dnssd = NULL;
        }
        else {
            dnssd->register_reply_cb = register_reply_cb;
            dnssd->service_discovered_cb = service_discovered_cb;
            dnssd->ctxt = context;
        }
    }

    return dnssd;
}

void purc_dnssd_disconnect(struct purc_dnssd_conn *dnssd)
{
    if (dnssd->shared_ref) {
        DNSServiceRefDeallocate(dnssd->shared_ref);
    }

    free(dnssd);
}

int purc_dnssd_fd(struct purc_dnssd_conn *dnssd)
{
    return DNSServiceRefSockFD(dnssd->shared_ref);
}

static void report_error(DNSServiceErrorType error)
{
    (void)error;
}

static uint16_t
make_txt_record_body(const char *txt_record_values[],
        size_t nr_txt_record_values, unsigned char *txt, size_t nr_txt_buf)
{
    DNSServiceErrorType error = kDNSServiceErr_NoError;
    unsigned char *ptr = txt;

    if (txt_record_values && nr_txt_record_values) {
        for (size_t i = 0; i < nr_txt_record_values; i++) {
            const char *p = txt_record_values[i];
            if (ptr >= txt + nr_txt_buf) {
                error = kDNSServiceErr_BadParam;
                goto failed;
            }

            *ptr = 0;
            while (*p && *ptr < 255) {
                if (ptr + 1 + *ptr >= txt + nr_txt_buf) {
                    error = kDNSServiceErr_BadParam;
                    goto failed;
                }

                if (p[0] != '\\' || p[1] == 0) {
                    ptr[++*ptr] = *p;
                    p+=1;
                }
                else if (p[1] == 'x' && isxdigit(p[2]) && isxdigit(p[3])) {
                    ptr[++*ptr] = HexPair(p+2);
                    p+=4;
                }
                else {
                    ptr[++*ptr] = p[1];
                    p+=2;
                }
            }
            ptr += 1 + *ptr;
        }
    }

    return (uint16_t)(ptr - txt);

failed:
    report_error(error);
    return 0;
}

static void register_reply(DNSServiceRef sdRef, DNSServiceFlags flags,
        DNSServiceErrorType errorCode, const char *name, const char *regtype,
        const char *domain, void *context)
{
    struct purc_dnssd_conn *dnssd = context;
    if (dnssd->register_reply_cb) {
        dnssd->register_reply_cb(dnssd, sdRef, flags, errorCode,
                name, regtype, domain, dnssd->ctxt);
    }
}

void *purc_dnssd_register_service(struct purc_dnssd_conn *dnssd,
        const char *service_name, const char *reg_type,
        const char *domain, const char *hostname, uint16_t port,
        const char *txt_record_values[], size_t nr_txt_record_values)
{
    unsigned char txt[MAX_TXT_RECORD_SIZE] = { 0 };
    uint16_t txt_len;
    txt_len = make_txt_record_body(txt_record_values, nr_txt_record_values,
            txt, sizeof(txt));

    DNSServiceRef regref = dnssd->shared_ref;
    DNSServiceFlags flags = kDNSServiceFlagsShareConnection;
    //flags |= kDNSServiceFlagsAllowRemoteQuery;
    //flags |= kDNSServiceFlagsNoAutoRenamee;

    Opaque16 registerPort = { { port >> 8, port & 0xFF } };

    DNSServiceErrorType error = DNSServiceRegister(&regref, flags,
            kDNSServiceInterfaceIndexAny,
            service_name, reg_type, domain, hostname,
            registerPort.NotAnInteger, txt_len, txt, register_reply, dnssd);
    if (error)
        return NULL;
    return regref;
}

void purc_dnssd_revoke_service(struct purc_dnssd_conn *dnssd,
        void *service_handle)
{
    (void)dnssd;
    DNSServiceRefDeallocate(service_handle);
}

static int
copy_labels(char *dst, const char *lim, const char **srcp, int labels)
{
    const char *src = *srcp;

    while (*src != '.' || --labels > 0) {
        if (*src == '\\')
            *dst++ = *src++;  // Make sure "\." doesn't confuse us
        if (!*src || dst >= lim)
            return -1;
        *dst++ = *src++;
        if (!*src || dst >= lim)
            return -1;
    }

    *dst++ = 0;
    *srcp = src + 1;    // skip over final dot
    return 0;
}

static void resolve_cb(DNSServiceRef sdref,
        const DNSServiceFlags flags, uint32_t if_index,
        DNSServiceErrorType error_code,
        const char *fullname, const char *hosttarget, uint16_t opaqueport,
        uint16_t txt_len, const unsigned char *txt, void *context)
{
    struct purc_dnssd_conn *dnssd = (struct purc_dnssd_conn *)context;
    union { uint16_t s; u_char b[2]; } port = { opaqueport };
    uint16_t u_port = ((uint16_t)port.b[0]) << 8 | port.b[1];
    const char *p = fullname;

    char n[MAX_DOMAIN_NAME] = "";
    char t[MAX_DOMAIN_NAME] = "";

    if (error_code == kDNSServiceErr_NoError) {
        // Fetch name+type
        if ((error_code = copy_labels(n, n + MAX_DOMAIN_NAME, &p, 3))) {
            goto done;
        }

        p = fullname;
        // Skip first label
        if ((error_code = copy_labels(t, t + MAX_DOMAIN_NAME, &p, 1))) {
            goto done;
        }
        // Fetch next two labels (service type)
        if ((error_code = copy_labels(t, t + MAX_DOMAIN_NAME, &p, 2))) {
            goto done;
        }

        const unsigned char *const end = txt + 1 + txt[0];
        txt++;      // Skip over length byte
        txt_len = (uint16_t)(end - txt);
    }

done:
    dnssd->service_discovered_cb(dnssd, sdref, flags, if_index, error_code,
        fullname, t, hosttarget, u_port, txt_len,
        (const char *)txt, dnssd->ctxt);
    DNSServiceRefDeallocate(sdref);
}

static void browse_cb(DNSServiceRef sdref, const DNSServiceFlags flags,
    uint32_t if_index, int error_code, const char *service_name,
    const char *reg_type, const char *reply_domain, void *ctxt)
{
    if (!(flags & kDNSServiceFlagsAdd)) {
        return;
    }

    struct purc_dnssd_conn *dnssd = (struct purc_dnssd_conn *)ctxt;
    if (error_code == kDNSServiceErr_NoError) {
        DNSServiceRef newref = dnssd->shared_ref;
        DNSServiceFlags my_flags = kDNSServiceFlagsShareConnection;

        DNSServiceResolve(&newref, my_flags, if_index, service_name, reg_type,
                reply_domain, resolve_cb, ctxt);
    }
    else {
        dnssd->service_discovered_cb(dnssd, sdref, flags, if_index, error_code,
                NULL, NULL, NULL, 0, 0, NULL,dnssd->ctxt);
    }
}

void *purc_dnssd_start_browsing(struct purc_dnssd_conn *dnssd,
        const char *reg_type, const char *domain)
{
    DNSServiceRef browse_ref;
    browse_ref = dnssd->shared_ref;
    DNSServiceFlags flags = kDNSServiceFlagsShareConnection;

    DNSServiceErrorType error = DNSServiceBrowse(&browse_ref,
            flags, kDNSServiceInterfaceIndexAny,
            reg_type, domain, browse_cb, dnssd);

    if (error) {
        return NULL;
    }

    return browse_ref;
}

void
purc_dnssd_stop_browsing(struct purc_dnssd_conn *dnssd, void *browsing_handle)
{
    (void)dnssd;
    DNSServiceRefDeallocate(browsing_handle);
}

int purc_dnssd_process_result(struct purc_dnssd_conn *dnssd)
{
    return DNSServiceProcessResult(dnssd->shared_ref);
}

#endif /* PCA_ENABLE_DNSSD */
