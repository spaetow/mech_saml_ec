/*
 * Copyright (c) 2010, JANET(UK)
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

#include "gssapiP_eap.h"

/*
 * 1.3.6.1.4.1.5322(padl)
 *      gssEap(21)
 *       mechanisms(1)
 *        eap-aes128-cts-hmac-sha1-96(17)
 *        eap-aes256-cts-hmac-sha1-96(18)
 *       nameTypes(2)
 *       apiExtensions(3)
 *        inquireSecContextByOid(1)
 *        inquireCredByOid(2)
 *        setSecContextOption(3)
 *        setCredOption(4)
 *        mechInvoke(5)
 */

/*
 * Prefix for GSS EAP mechanisms. A Kerberos encryption type is
 * concatenated with this to form a concrete mechanism OID.
 */
static const gss_OID_desc gssEapMechPrefix = {
    /* 1.3.6.1.4.1.5322.21.1 */
    11, "\x06\x09\x2B\x06\x01\x04\x01\xA9\x4A\x15\x01"
};

const gss_OID_desc *const gss_mech_eap = &gssEapMechPrefix;

static const gss_OID_desc gssEapConcreteMechs[] = {
    /* 1.3.6.1.4.1.5322.21.1.17 */
    { 12, "\x06\x0A\x2B\x06\x01\x04\x01\xA9\x4A\x15\x01\x11" },
    /* 1.3.6.1.4.1.5322.21.1.18 */
    { 12, "\x06\x0A\x2B\x06\x01\x04\x01\xA9\x4A\x15\x01\x12" }
};

const gss_OID_desc *const gss_mech_eap_aes128_cts_hmac_sha1_96 =
    &gssEapConcreteMechs[0];
const gss_OID_desc *const gss_mech_eap_aes256_cts_hmac_sha1_96 =
    &gssEapConcreteMechs[1];

OM_uint32
gssEapOidToEnctype(OM_uint32 *minor,
                   const gss_OID oid,
                   krb5_enctype *enctype)
{
    OM_uint32 major;
    int suffix;

    major = decomposeOid(minor,
                         gssEapMechPrefix.elements,
                         gssEapMechPrefix.length,
                         oid,
                         &suffix);
    if (major == GSS_S_COMPLETE)
        *enctype = suffix;

    return major;
}

OM_uint32
gssEapEnctypeToOid(OM_uint32 *minor,
                   krb5_enctype enctype,
                   gss_OID *pOid)
{
    OM_uint32 major;
    gss_OID oid;

    *pOid = NULL;

    oid = (gss_OID)GSSEAP_MALLOC(sizeof(*oid));
    if (oid == NULL) {
        *minor = ENOMEM;
        return GSS_S_FAILURE;
    }

    oid->elements = GSSEAP_MALLOC(gssEapMechPrefix.length + 2);
    if (oid->elements == NULL) {
        *minor = ENOMEM;
        free(oid);
        return GSS_S_FAILURE;
    }

    major = composeOid(minor,
                       gssEapMechPrefix.elements,
                       gssEapMechPrefix.length,
                       enctype,
                       oid);
    if (major == GSS_S_COMPLETE) {
        gssEapInternalizeOid(oid, pOid);
        *pOid = oid;
    } else {
        free(oid->elements);
        free(oid);
    }

    return major;
}

OM_uint32
gssEapIndicateMechs(OM_uint32 *minor,
                    gss_OID_set *mechs)
{
    krb5_context context;
    OM_uint32 major, tmpMinor;
    krb5_enctype *etypes;
    int i;

    *minor = krb5_init_context(&context);
    if (*minor != 0) {
        return GSS_S_FAILURE;
    }

    *minor = krb5_get_permitted_enctypes(context, &etypes);
    if (*minor != 0) {
        krb5_free_context(context);
        return GSS_S_FAILURE;
    }

    major = gss_create_empty_oid_set(minor, mechs);
    if (GSS_ERROR(major)) {
        krb5_free_context(context);
        GSSEAP_FREE(etypes); /* XXX */
        return major;
    }

    for (i = 0; etypes[i] != ENCTYPE_NULL; i++) {
        gss_OID mechOid;

        major = gssEapEnctypeToOid(minor, etypes[i], &mechOid);
        if (GSS_ERROR(major))
            break;

        major = gss_add_oid_set_member(minor, mechOid, mechs);
        if (GSS_ERROR(major))
            break;

        gss_release_oid(&tmpMinor, &mechOid);
    }

    GSSEAP_FREE(etypes); /* XXX */
    krb5_free_context(context);

    return major;
}

OM_uint32
gssEapDefaultMech(OM_uint32 *minor,
                  gss_OID *oid)
{
    gss_OID_set mechs;
    OM_uint32 major, tmpMinor;

    major = gssEapIndicateMechs(minor, &mechs);
    if (GSS_ERROR(major)) {
        return major;
    }

    if (mechs->count == 0) {
        gss_release_oid_set(&tmpMinor, &mechs);
        return GSS_S_BAD_MECH;
    }

    gssEapInternalizeOid(&mechs->elements[0], oid);
    if (*oid == &mechs->elements[0]) {
        /* don't double-free if we didn't internalize it */
        mechs->elements[0].length = 0;
        mechs->elements[0].elements = NULL;
    }

    gss_release_oid_set(&tmpMinor, &mechs);

    *minor = 0;
    return GSS_S_COMPLETE;
}

void
gssEapInternalizeOid(const gss_OID oid,
                     gss_OID *const pInternalizedOid)
{
    int i;

    *pInternalizedOid = GSS_C_NO_OID;

    if (oidEqual(oid, &gssEapMechPrefix)) {
        *pInternalizedOid = (const gss_OID)&gssEapMechPrefix;
    } else {
        for (i = 0;
             i < sizeof(gssEapConcreteMechs) / sizeof(gssEapConcreteMechs[0]);
             i++) {
            if (oidEqual(oid, &gssEapConcreteMechs[i])) {
                *pInternalizedOid = (const gss_OID)&gssEapConcreteMechs[i];
                break;
            }
        }
    }

    if (*pInternalizedOid == GSS_C_NO_OID) {
        *pInternalizedOid = oid;
    }
}