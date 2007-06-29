/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */


#pragma ident	"%Z%%M%	%I%	%E% SMI"

#if defined(_KERNEL)
#include <sys/systm.h>
#include <sys/sunddi.h>
#include <sys/ctype.h>
#else
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <libnvpair.h>
#include <ctype.h>
#endif
#include <sys/dsl_deleg.h>
#include "zfs_deleg.h"
#include "zfs_namecheck.h"

/*
 * permission table
 */

char *zfs_deleg_perm_tab[] = {
	ZFS_DELEG_PERM_CREATE,
	ZFS_DELEG_PERM_DESTROY,
	ZFS_DELEG_PERM_SNAPSHOT,
	ZFS_DELEG_PERM_ROLLBACK,
	ZFS_DELEG_PERM_CLONE,
	ZFS_DELEG_PERM_PROMOTE,
	ZFS_DELEG_PERM_RENAME,
	ZFS_DELEG_PERM_MOUNT,
	ZFS_DELEG_PERM_SHARE,
	ZFS_DELEG_PERM_SEND,
	ZFS_DELEG_PERM_RECEIVE,
	ZFS_DELEG_PERM_QUOTA,
	ZFS_DELEG_PERM_RESERVATION,
	ZFS_DELEG_PERM_VOLSIZE,
	ZFS_DELEG_PERM_RECORDSIZE,
	ZFS_DELEG_PERM_MOUNTPOINT,
	ZFS_DELEG_PERM_SHARENFS,
	ZFS_DELEG_PERM_CHECKSUM,
	ZFS_DELEG_PERM_COMPRESSION,
	ZFS_DELEG_PERM_ATIME,
	ZFS_DELEG_PERM_DEVICES,
	ZFS_DELEG_PERM_EXEC,
	ZFS_DELEG_PERM_SETUID,
	ZFS_DELEG_PERM_READONLY,
	ZFS_DELEG_PERM_ZONED,
	ZFS_DELEG_PERM_SNAPDIR,
	ZFS_DELEG_PERM_ACLMODE,
	ZFS_DELEG_PERM_ACLINHERIT,
	ZFS_DELEG_PERM_ALLOW,
	ZFS_DELEG_PERM_CANMOUNT,
	ZFS_DELEG_PERM_USERPROP,
	ZFS_DELEG_PERM_SHAREISCSI,
	ZFS_DELEG_PERM_XATTR,
	ZFS_DELEG_PERM_COPIES,
	ZFS_DELEG_PERM_VERSION,
	NULL
};

int
zfs_deleg_type(char *name)
{
	return (name[0]);
}

static int
zfs_valid_permission_name(char *perm)
{
	int i;
	for (i = 0; zfs_deleg_perm_tab[i] != NULL; i++) {
		if (strcmp(perm, zfs_deleg_perm_tab[i]) == 0)
			return (0);
	}

	return (permset_namecheck(perm, NULL, NULL));
}

static int
zfs_validate_who(char *who)
{
	int error = 0;
	char *p;

	switch (zfs_deleg_type(who)) {
	case ZFS_DELEG_USER:
	case ZFS_DELEG_GROUP:
	case ZFS_DELEG_USER_SETS:
	case ZFS_DELEG_GROUP_SETS:
		if ((who[1] != 'l' || who[1] != 'd') &&
		    (who[2] != ZFS_DELEG_FIELD_SEP_CHR)) {
			error = -1;
			break;
		}

		for (p = &who[3]; p && *p; p++)
			if (!isdigit(*p)) {
				error = -1;
			}
		break;
	case ZFS_DELEG_NAMED_SET:
	case ZFS_DELEG_NAMED_SET_SETS:
		error =  permset_namecheck(&who[3], NULL, NULL);
		break;

	case ZFS_DELEG_CREATE:
	case ZFS_DELEG_CREATE_SETS:
	case ZFS_DELEG_EVERYONE:
	case ZFS_DELEG_EVERYONE_SETS:
		if (who[3] != '\0')
			error = -1;
		break;
	default:
		error = -1;
	}

	return (error);
}

static int
zfs_validate_iflags(char *who)
{
	switch (zfs_deleg_type(who)) {
	case ZFS_DELEG_NAMED_SET:
	case ZFS_DELEG_NAMED_SET_SETS:
	case ZFS_DELEG_CREATE:
	case ZFS_DELEG_CREATE_SETS:
		if (who[1] != '-')
			return (-1);
		break;
	default:
		if (who[1] != 'l' && who[1] != 'd')
			return (-1);
		break;
	}
	return (0);
}

int
zfs_deleg_verify_nvlist(nvlist_t *nvp)
{
	nvpair_t *who, *perm_name;
	nvlist_t *perms;
	int error;

	if (nvp == NULL)
		return (-1);

	who = nvlist_next_nvpair(nvp, NULL);
	if (who == NULL)
		return (-1);

	do {
		if (zfs_validate_who(nvpair_name(who)))
			return (-1);

		if (zfs_validate_iflags(nvpair_name(who)))
			return (-1);

		error = nvlist_lookup_nvlist(nvp, nvpair_name(who), &perms);

		if (error && error != ENOENT)
			return (-1);
		if (error == ENOENT)
			continue;

		perm_name = nvlist_next_nvpair(perms, NULL);
		if (perm_name == NULL) {
			return (-1);
		}
		do {
			error = zfs_valid_permission_name(
			    nvpair_name(perm_name));
			if (error) {
				return (-1);
			}
		} while (perm_name = nvlist_next_nvpair(perms, perm_name));
	} while (who = nvlist_next_nvpair(nvp, who));
	return (0);
}

/*
 * Construct the base attribute name.  The base attribute names
 * are the "key" to locate the jump objects which contain the actual
 * permissions.  The base attribute names are encoded based on
 * type of entry and whether it is a local or descendent permission.
 *
 * Arguments:
 * attr - attribute name return string, attribute is assumed to be
 *        ZFS_MAX_DELEG_NAME long.
 * type - type of entry to construct
 * inheritchr - inheritance type (local,descendent, or NA for create and
 *                               permission set definitions
 * data - is either a permission set name or a 64 bit uid/gid.
 */
void
zfs_deleg_whokey(char *attr, char type, char inheritchr, void *data)
{
	int len = ZFS_MAX_DELEG_NAME;
	uint64_t *id = data;

	switch (type) {
	case ZFS_DELEG_USER:
	case ZFS_DELEG_GROUP:
	case ZFS_DELEG_USER_SETS:
	case ZFS_DELEG_GROUP_SETS:
		(void) snprintf(attr, len, "%c%c%c%lld", type, inheritchr,
		    ZFS_DELEG_FIELD_SEP_CHR, (longlong_t)*id);
		break;
	case ZFS_DELEG_NAMED_SET_SETS:
	case ZFS_DELEG_NAMED_SET:
		(void) snprintf(attr, len, "%c-%c%s", type,
		    ZFS_DELEG_FIELD_SEP_CHR, (char *)data);
		break;
	case ZFS_DELEG_CREATE:
	case ZFS_DELEG_CREATE_SETS:
		(void) snprintf(attr, len, "%c-%c", type,
		    ZFS_DELEG_FIELD_SEP_CHR);
		break;
	default:
		(void) snprintf(attr, len, "%c%c%c", type, inheritchr,
		    ZFS_DELEG_FIELD_SEP_CHR);
	}
}
