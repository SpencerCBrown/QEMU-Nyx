/*
 * QEMU Block backends
 *
 * Copyright (C) 2014-2016 Red Hat, Inc.
 *
 * Authors:
 *  Markus Armbruster <armbru@redhat.com>,
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1
 * or later.  See the COPYING.LIB file in the top-level directory.
 */

#ifndef BLOCK_BACKEND_H
#define BLOCK_BACKEND_H

#include "block-backend-global-state.h"
#include "block-backend-io.h"

/* DO NOT ADD ANYTHING IN HERE. USE ONE OF THE HEADERS INCLUDED ABOVE */

#ifdef QEMU_NYX
struct BlockBackend {
    cow_cache_t* cow_cache;
    char *name;
    int refcnt;
    BdrvChild *root;
    AioContext *ctx;
    DriveInfo *legacy_dinfo;    /* null unless created by drive_new() */
    QTAILQ_ENTRY(BlockBackend) link;         /* for block_backends */
    QTAILQ_ENTRY(BlockBackend) monitor_link; /* for monitor_block_backends */
    BlockBackendPublic public;

    DeviceState *dev;           /* attached device model, if any */
    const BlockDevOps *dev_ops;
    void *dev_opaque;

    /* the block size for which the guest device expects atomicity */
    int guest_block_size;

    /* If the BDS tree is removed, some of its options are stored here (which
     * can be used to restore those options in the new BDS on insert) */
    BlockBackendRootState root_state;

    bool enable_write_cache;

    /* I/O stats (display with "info blockstats"). */
    BlockAcctStats stats;

    BlockdevOnError on_read_error, on_write_error;
    bool iostatus_enabled;
    BlockDeviceIoStatus iostatus;

    uint64_t perm;
    uint64_t shared_perm;
    bool disable_perm;

    bool allow_aio_context_change;
    bool allow_write_beyond_eof;

    NotifierList remove_bs_notifiers, insert_bs_notifiers;
    QLIST_HEAD(, BlockBackendAioNotifier) aio_notifiers;

    int quiesce_counter;
    CoQueue queued_requests;
    bool disable_request_queuing;

    VMChangeStateEntry *vmsh;
    bool force_allow_inactivate;

    /* Number of in-flight aio requests.  BlockDriverState also counts
     * in-flight requests but aio requests can exist even when blk->root is
     * NULL, so we cannot rely on its counter for that case.
     * Accessed with atomic ops.
     */
    unsigned int in_flight;
};
#endif

#endif
