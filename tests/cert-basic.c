/* cert-basic.c - basic test for the certificate management.
 *      Copyright (C) 2001, 2002, 2004, 2005 g10 Code GmbH
 *
 * This file is part of KSBA.
 *
 * KSBA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * KSBA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "../src/ksba.h"
#define _KSBA_VISIBILITY_DEFAULT /*  */
#include "../src/keyinfo.h"

#include "oidtranstbl.h"
#include "t-common.h"

#define digitp(p)   (*(p) >= '0' && *(p) <= '9')

#define fail_if_err(a) do { if(a) {                                       \
                              fprintf (stderr, "%s:%d: KSBA error: %s\n", \
                              __FILE__, __LINE__, gpg_strerror(a));   \
                              exit (1); }                              \
                           } while(0)


#define fail_if_err2(f, a) do { if(a) {\
            fprintf (stderr, "%s:%d: KSBA error on file `%s': %s\n", \
                       __FILE__, __LINE__, (f), gpg_strerror(a));   \
                            exit (1); }                              \
                           } while(0)

#define xfree(a)  ksba_free (a)

static int quiet;
static int verbose;
static int errorcount;


static void
print_names (int indent, ksba_name_t name)
{
  int idx;
  const char *s;
  int indent_all;

  if ((indent_all = (indent < 0)))
    indent = - indent;

  if (!name)
    {
      if (!quiet)
        fputs ("none\n", stdout);
      return;
    }

  for (idx=0; (s = ksba_name_enum (name, idx)); idx++)
    {
      char *p = ksba_name_get_uri (name, idx);
      if (!quiet)
        printf ("%*s%s\n", idx||indent_all?indent:0, "", p?p:s);
      xfree (p);
    }
}


/* Return the description for OID; if no description is available
   NULL is returned. */
static const char *
get_oid_desc (const char *oid)
{
  int i;

  if (oid)
    for (i=0; oidtranstbl[i].oid; i++)
      if (!strcmp (oidtranstbl[i].oid, oid))
        return oidtranstbl[i].desc;
  return NULL;
}


static void
print_oid_and_desc (const char *oid, int with_lf)
{
  const char *s = get_oid_desc (oid);
  printf ("%s%s%s%s",
          oid, s?" (":"", s?s:"", s?")":"");
  if (with_lf)
    putchar ('\n');
}


static void
print_oid_list (int indent, char *list)
{
  char *lf;
  int indent_all, c;
  size_t n;

  if ((indent_all = (indent < 0)))
    indent = - indent;

  while (*list)
    {
      printf ("%*s", indent_all?indent:0, "");
      indent_all = 1;

      if (!(lf = strchr (list, '\n')))
        lf = list + strlen (list);

      n = strspn (list, "0123456789.");
      c = list[n];
      list[n] = 0;
      print_oid_and_desc (list, 0);
      list[n] = c;

      c = *lf;
      *lf = 0;
      printf ("  %s\n", list+n);
      *lf = c;
      list = *lf? (lf+1):lf;
    }
}




static void
list_extensions (ksba_cert_t cert)
{
  gpg_error_t err;
  const char *oid;
  int idx, crit, is_ca, pathlen;
  size_t off, len;
  unsigned int usage, reason;
  char *string, *p;
  ksba_name_t name1, name2;
  ksba_sexp_t serial;
  ksba_sexp_t keyid;

  for (idx=0; !(err=ksba_cert_get_extension (cert, idx,
                                             &oid, &crit, &off, &len));idx++)
    {
      const char *s = get_oid_desc (oid);
      if (!quiet)
        printf ("Extn: %s%s%s%s at %d with length %d %s\n",
                oid, s?" (":"", s?s:"", s?")":"",
                (int)off, (int)len, crit? "(critical)":"");
    }
  if (err && gpg_err_code (err) != GPG_ERR_EOF )
    {
      fprintf (stderr,
               "%s:%d: enumerating extensions failed: %s\n",
               __FILE__, __LINE__, gpg_strerror (err));
      errorcount++;
    }

  /* subjectKeyIdentifier */
  err = ksba_cert_get_subj_key_id (cert, NULL, &keyid);
  if (!err || gpg_err_code (err) == GPG_ERR_NO_DATA)
    {
      if (!quiet)
        fputs ("SubjectKeyIdentifier: ", stdout);
      if (gpg_err_code (err) == GPG_ERR_NO_DATA)
        {
          if (!quiet)
            fputs ("none\n", stdout);
        }
      else
        {
          if (!quiet)
            {
              print_sexp (keyid);
              putchar ('\n');
            }
        }
      ksba_free (keyid);
    }


  /* authorityKeyIdentifier */
  err = ksba_cert_get_auth_key_id (cert, &keyid, &name1, &serial);
  if (!err || gpg_err_code (err) == GPG_ERR_NO_DATA)
    {
      if (!quiet)
        fputs ("AuthorityKeyIdentifier: ", stdout);
      if (gpg_err_code (err) == GPG_ERR_NO_DATA)
        {
          if (!quiet)
            fputs ("none\n", stdout);
        }
      else
        {
          if (name1)
            {
              print_names (24, name1);
              ksba_name_release (name1);
              if (!quiet)
                {
                  fputs ("                serial: ", stdout);
                  print_sexp (serial);
                }
              ksba_free (serial);
            }
          if (!quiet)
            putchar ('\n');
          if (keyid)
            {
              if (!quiet)
                {
                  fputs ("         keyIdentifier: ", stdout);
                  print_sexp (keyid);
                  putchar ('\n');
                }
              ksba_free (keyid);
            }
        }
    }
  else
    {
      fprintf (stderr, "%s:%d: ksba_cert_get_auth_key_id: %s\n",
               __FILE__, __LINE__, gpg_strerror (err));
      errorcount++;
    }

  err = ksba_cert_is_ca (cert, &is_ca, &pathlen);
  if (err)
    {
      fprintf (stderr, "%s:%d: ksba_cert_is_ca failed: %s\n",
               __FILE__, __LINE__, gpg_strerror (err));
      errorcount++;
    }
  else if (is_ca)
    {
      if (!quiet)
        printf ("This is a CA certificate with a path length of %d\n", pathlen);
    }

  err = ksba_cert_get_key_usage (cert, &usage);
  if (gpg_err_code (err) == GPG_ERR_NO_DATA)
    {
      if (!quiet)
        printf ("KeyUsage: Not specified\n");
    }
  else if (err)
    {
      fprintf (stderr, "%s:%d: ksba_cert_get_key_usage failed: %s\n",
               __FILE__, __LINE__, gpg_strerror (err));
      errorcount++;
    }
  else if (!quiet)
    {
      fputs ("KeyUsage:", stdout);
      if ( (usage & KSBA_KEYUSAGE_DIGITAL_SIGNATURE))
        fputs (" digitalSignature", stdout);
      if ( (usage & KSBA_KEYUSAGE_NON_REPUDIATION))
        fputs (" nonRepudiation", stdout);
      if ( (usage & KSBA_KEYUSAGE_KEY_ENCIPHERMENT))
        fputs (" keyEncipherment", stdout);
      if ( (usage & KSBA_KEYUSAGE_DATA_ENCIPHERMENT))
        fputs (" dataEncripherment", stdout);
      if ( (usage & KSBA_KEYUSAGE_KEY_AGREEMENT))
        fputs (" keyAgreement", stdout);
      if ( (usage & KSBA_KEYUSAGE_KEY_CERT_SIGN))
        fputs (" certSign", stdout);
      if ( (usage & KSBA_KEYUSAGE_CRL_SIGN))
        fputs (" crlSign", stdout);
      if ( (usage & KSBA_KEYUSAGE_ENCIPHER_ONLY))
        fputs (" encipherOnly", stdout);
      if ( (usage & KSBA_KEYUSAGE_DECIPHER_ONLY))
        fputs (" decipherOnly", stdout);
      putchar ('\n');
    }
  err = ksba_cert_get_ext_key_usages (cert, &string);
  if (gpg_err_code (err) == GPG_ERR_NO_DATA)
    {
      if (!quiet)
        printf ("ExtKeyUsages: none\n");
    }
  else if (err)
    {
      fprintf (stderr, "%s:%d: ksba_cert_ext_key_usages failed: %s\n",
               __FILE__, __LINE__, gpg_strerror (err));
      errorcount++;
    }
  else
    {
      if (!quiet)
        {
          fputs ("ExtKeyUsages: ", stdout);
          print_oid_list (14, string);
        }
      xfree (string);
    }


  err = ksba_cert_get_cert_policies (cert, &string);
  if (gpg_err_code (err) == GPG_ERR_NO_DATA)
    {
      if (!quiet)
        printf ("CertificatePolicies: none\n");
    }
  else if (err)
    {
      fprintf (stderr, "%s:%d: ksba_cert_get_cert_policies failed: %s\n",
               __FILE__, __LINE__, gpg_strerror (err));
      errorcount++;
    }
  else
    {
      if (!quiet)
        {
          /* for display purposes we replace the linefeeds by commas */
          for (p=string; *p; p++)
            {
              if (*p == '\n')
                *p = ',';
            }
          fputs ("CertificatePolicies: ", stdout);
          print_oid_list (21, string);
        }
      xfree (string);
    }

  /* CRL distribution point */
  for (idx=0; !(err=ksba_cert_get_crl_dist_point (cert, idx,
                                                  &name1, &name2,
                                                  &reason));idx++)
    {
      if (!quiet)
        fputs ("CRLDistPoint: ", stdout);
      print_names (14, name1);
      if (!quiet)
        {
          fputs ("     reasons:", stdout);
          if ( !reason )
            fputs (" none", stdout);
          if ( (reason & KSBA_CRLREASON_UNSPECIFIED))
            fputs (" unused", stdout);
          if ( (reason & KSBA_CRLREASON_KEY_COMPROMISE))
            fputs (" keyCompromise", stdout);
          if ( (reason & KSBA_CRLREASON_CA_COMPROMISE))
            fputs (" caCompromise", stdout);
          if ( (reason & KSBA_CRLREASON_AFFILIATION_CHANGED))
            fputs (" affiliationChanged", stdout);
          if ( (reason & KSBA_CRLREASON_SUPERSEDED))
            fputs (" superseded", stdout);
          if ( (reason & KSBA_CRLREASON_CESSATION_OF_OPERATION))
            fputs (" cessationOfOperation", stdout);
          if ( (reason & KSBA_CRLREASON_CERTIFICATE_HOLD))
            fputs (" certificateHold", stdout);
          putchar ('\n');
          fputs ("      issuer: ", stdout);
        }
      print_names (14, name2);
      ksba_name_release (name1);
      ksba_name_release (name2);
    }
  if (err && gpg_err_code (err) != GPG_ERR_EOF)
    {
      fprintf (stderr, "%s:%d: ksba_cert_get_crl_dist_point failed: %s\n",
               __FILE__, __LINE__, gpg_strerror (err));
      errorcount++;
    }

  /* authorityInfoAccess. */
  for (idx=0; !(err=ksba_cert_get_authority_info_access (cert, idx,
                                                         &string, &name1))
       ; idx++)
    {
      if (!quiet)
        {
          fputs ("authorityInfoAccess: ", stdout);
          print_oid_and_desc (string, 1);
        }
      print_names (-21, name1);
      ksba_name_release (name1);
      ksba_free (string);
    }
  if (err && gpg_err_code (err) != GPG_ERR_EOF)
    {
      fprintf (stderr, "%s:%d: "
               "ksba_cert_get_authority_info_access failed: %s\n",
               __FILE__, __LINE__, gpg_strerror (err));
      errorcount++;
    }

  /* subjectInfoAccess. */
  for (idx=0; !(err=ksba_cert_get_subject_info_access (cert, idx,
                                                       &string, &name1))
       ; idx++)
    {
      if (!quiet)
        {
          fputs ("subjectInfoAccess: ", stdout);
          print_oid_and_desc (string, 1);
        }
      print_names (-19, name1);
      ksba_name_release (name1);
      ksba_free (string);
    }
  if (err && gpg_err_code (err) != GPG_ERR_EOF)
    {
      fprintf (stderr, "%s:%d: "
               "ksba_cert_get_subject_info_access failed: %s\n",
               __FILE__, __LINE__, gpg_strerror (err));
      errorcount++;
    }

}


static void
one_file (const char *fname)
{
  gpg_error_t err;
  FILE *fp;
  ksba_reader_t r;
  ksba_cert_t cert;
  char *dn;
  ksba_isotime_t t;
  int idx;
  ksba_sexp_t sexp;
  const char *oid, *s;

  fp = fopen (fname, "rb");
  if (!fp)
    {
      fprintf (stderr, "%s:%d: can't open `%s': %s\n",
               __FILE__, __LINE__, fname, strerror (errno));
      exit (1);
    }

  err = ksba_reader_new (&r);
  if (err)
    fail_if_err (err);
  err = ksba_reader_set_file (r, fp);
  fail_if_err (err);

  err = ksba_cert_new (&cert);
  if (err)
    fail_if_err (err);

  err = ksba_cert_read_der (cert, r);
  fail_if_err2 (fname, err);

  if (!quiet)
    printf ("Certificate in `%s':\n", fname);

  sexp = ksba_cert_get_serial (cert);
  if (!quiet)
    {
      fputs ("  serial....: ", stdout);
      print_sexp (sexp);
      putchar ('\n');
    }
  ksba_free (sexp);

  for (idx=0;(dn = ksba_cert_get_issuer (cert, idx));idx++)
    {
      if (!quiet)
        {
          fputs (idx?"         aka: ":"  issuer....: ", stdout);
          print_dn (dn);
          putchar ('\n');
        }
      ksba_free (dn);
    }

  for (idx=0;(dn = ksba_cert_get_subject (cert, idx));idx++)
    {
      if (!quiet)
        {
          fputs (idx?"         aka: ":"  subject...: ", stdout);
          print_dn (dn);
          putchar ('\n');
        }
      ksba_free (dn);
    }

  ksba_cert_get_validity (cert, 0, t);
  if (!quiet)
    {
      fputs ("  notBefore.: ", stdout);
      print_time (t);
      putchar ('\n');
    }
  ksba_cert_get_validity (cert, 1, t);
  if (!quiet)
    {
      fputs ("  notAfter..: ", stdout);
      print_time (t);
      putchar ('\n');
    }
  oid = ksba_cert_get_digest_algo (cert);
  s = get_oid_desc (oid);
  if (!quiet)
    printf ("  hash algo.: %s%s%s%s\n",
            oid?oid:"(null)", s?" (":"",s?s:"",s?")":"");

  /* Under Windows the _ksba_keyinfo_from_sexp are not exported.  */
#ifndef __WIN32
  /* check that the sexp to keyinfo conversion works */
  {
    ksba_sexp_t public;

    public = ksba_cert_get_public_key (cert);
    if (!public)
      {
        fprintf (stderr, "%s:%d: public key not found\n",
                 __FILE__, __LINE__);
        errorcount++;
      }
    else
      {
        unsigned char *der;
        size_t derlen;

        if (verbose)
          {
            fputs ("  pubkey....: ", stdout);
            print_sexp (public);
            putchar ('\n');
          }

        err = _ksba_keyinfo_from_sexp (public, 0, &der, &derlen);
        if (err)
          {
            fprintf (stderr, "%s:%d: converting public key failed: %s\n",
                     __FILE__, __LINE__, gpg_strerror (err));
            errorcount++;
          }
        else
          {
            ksba_sexp_t tmp;

            if (verbose)
              {
                fputs ("  pubkey-DER: ", stdout);
                print_hex (der, derlen);
                putchar ('\n');
              }
            err = _ksba_keyinfo_to_sexp (der, derlen, &tmp);
            if (err)
              {
                fprintf (stderr,
                         "%s:%d: re-converting public key failed: %s\n",
                         __FILE__, __LINE__, gpg_strerror (err));
                errorcount++;
              }
            else
              {
                unsigned char *der2;
                size_t derlen2;

                err = _ksba_keyinfo_from_sexp (tmp, 0, &der2, &derlen2);
                if (err)
                  {
                    fprintf (stderr, "%s:%d: re-re-converting "
                             "public key failed: %s\n",
                             __FILE__, __LINE__, gpg_strerror (err));
                    errorcount++;
                  }
                else if (derlen != derlen2 || memcmp (der, der2, derlen))
                  {
                    fprintf (stderr, "%s:%d: mismatch after "
                             "re-re-converting public key\n",
                             __FILE__, __LINE__);
                    errorcount++;
                    xfree (der2);
                  }
                else
                  {
                    /* Don't leak memory if everything is ok. */
                    xfree (der2);
                  }
                xfree (tmp);
              }
            xfree (der);
          }
        ksba_free (public);
      }
  }
#endif

  if (verbose)
    {
      sexp = ksba_cert_get_sig_val (cert);
      fputs ("  sigval....: ", stdout);
      print_sexp (sexp);
      putchar ('\n');
      ksba_free (sexp);
    }

  list_extensions (cert);

  ksba_cert_release (cert);
  err = ksba_cert_new (&cert);
  if (err)
    fail_if_err (err);

  err = ksba_cert_read_der (cert, r);
  if (err && gpg_err_code (err) != GPG_ERR_EOF)
    {
      fprintf (stderr, "%s:%d: expected EOF but got: %s\n",
               __FILE__, __LINE__, gpg_strerror (err));
      errorcount++;
    }

  if (!quiet)
    putchar ('\n');
  ksba_cert_release (cert);
  ksba_reader_release (r);
  fclose (fp);
}




int
main (int argc, char **argv)
{
  const char *srcdir = getenv ("srcdir");

  if (!srcdir)
    srcdir = ".";

  if (argc)
    {
      argc--; argv++;
    }

  if (argc && !strcmp (*argv, "--verbose"))
    {
      verbose = 1;
      argc--; argv++;
    }


  if (argc)
    {
      for (; argc; argc--, argv++)
        one_file (*argv);
    }
  else
    {
      const char *files[] = {
        "cert_dfn_pca01.der",
        "cert_dfn_pca15.der",
        "cert_g10code_test1.der",
        "authority.crt",
        "betsy.crt",
        "bull.crt",
        "ov-ocsp-server.crt",
        "ov-userrev.crt",
        "ov-root-ca-cert.crt",
        "ov-serverrev.crt",
        "ov-user.crt",
        "ov-server.crt",
        "ov2-root-ca-cert.crt",
        "ov2-ocsp-server.crt",
        "ov2-user.crt",
        "ov2-userrev.crt",
        "secp256r1-sha384_cert.crt",
        "secp256r1-sha512_cert.crt",
        "secp384r1-sha512_cert.crt",
        "openssl-secp256r1ca.cert.crt",
        "ed25519-rfc8410.crt",
        "ed25519-ossl-1.crt",
        "ed448-ossl-1.crt",
        NULL
      };
      int idx;

      if (!verbose)
        quiet = 1;

      for (idx=0; files[idx]; idx++)
        {
          char *fname;

          fname = xmalloc (strlen (srcdir) + 10 + strlen (files[idx]) + 1);
          strcpy (fname, srcdir);
          strcat (fname, "/samples/");
          strcat (fname, files[idx]);
          one_file (fname);
          ksba_free (fname);
        }
    }

  return !!errorcount;
}
