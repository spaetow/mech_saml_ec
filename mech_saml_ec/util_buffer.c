/*
 * Copyright (c) 2011, JANET(UK)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of JANET(UK) nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Buffer handling helpers.
 */

#include "gssapiP_eap.h"

int mech_saml_ec_debug = -1;

OM_uint32
makeStringBuffer(OM_uint32 *minor,
                 const char *string,
                 gss_buffer_t buffer)
{
    size_t len = strlen(string);

    buffer->value = GSSEAP_MALLOC(len + 1);
    if (buffer->value == NULL) {
        *minor = ENOMEM;
        return GSS_S_FAILURE;
    }
    memcpy(buffer->value, string, len + 1);
    buffer->length = len;

    *minor = 0;
    return GSS_S_COMPLETE;
}

OM_uint32
addToStringBuffer(OM_uint32 *minor,
                 const char *ptr,
                 const size_t len,
                 gss_buffer_t buffer)
{
    buffer->value = GSSEAP_REALLOC(buffer->value, buffer->length + len + 1);
    if (buffer->value == NULL) {
        *minor = ENOMEM;
        return GSS_S_FAILURE;
    }
    memcpy(buffer->value+buffer->length, ptr, len);
    buffer->length += len;
    ((char *)buffer->value)[buffer->length] = '\0';

    *minor = 0;
    return GSS_S_COMPLETE;
}

OM_uint32
bufferToString(OM_uint32 *minor,
               const gss_buffer_t buffer,
               char **pString)
{
    char *s;

    s = GSSEAP_MALLOC(buffer->length + 1);
    if (s == NULL) {
        *minor = ENOMEM;
        return GSS_S_FAILURE;
    }
    memcpy(s, buffer->value, buffer->length);
    s[buffer->length] = '\0';

    *pString = s;

    *minor = 0;
    return GSS_S_COMPLETE;
}

OM_uint32
duplicateBuffer(OM_uint32 *minor,
                const gss_buffer_t src,
                gss_buffer_t dst)
{
    dst->length = 0;
    dst->value = NULL;

    if (src == GSS_C_NO_BUFFER)
        return GSS_S_COMPLETE;

    dst->value = GSSEAP_MALLOC(src->length + 1);
    if (dst->value == NULL) {
        *minor = ENOMEM;
        return GSS_S_FAILURE;
    }

    dst->length = src->length;
    memcpy(dst->value, src->value, dst->length);

    ((unsigned char *)dst->value)[dst->length] = '\0';

    *minor = 0;
    return GSS_S_COMPLETE;
}

void
printBuffer(FILE *stream, const gss_buffer_t src)
{
    int i = 0;

    if (src == GSS_C_NO_BUFFER)
        return;

    fprintf(stream, "BYTES IN TOKEN ARE: (");
    for (i = 0; i < src->length; i++)
    {
        fprintf(stream, "%x ", ((unsigned char *)src->value)[i]);
    }
    fprintf(stream, ")\n");
}
