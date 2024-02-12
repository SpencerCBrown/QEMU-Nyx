/*
 * QEMU I/O channels memory buffer driver
 *
 * Copyright (c) 2015 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "qemu/osdep.h"
#include "io/channel-watch.h"
#include "qemu/module.h"
#include "qemu/sockets.h"
#include "trace.h"

#include "nyx_qio_buffer.h"

NYXQIOBuffer *
nyx_qio_buffer_new(struct fast_savevm_opaque_t* opaque)
{
    NYXQIOBuffer *ioc;

    ioc = NYX_QIO_BUFFER(object_new(TYPE_NYX_QIO_BUFFER));

    // no need to allocate anything, that should already be done when this is constructed
    ioc->opaque = opaque;

    return ioc;
}


static void nyx_qio_buffer_finalize(Object *obj)
{
    // no need to free anything, memory isn't managed by this object
    return;
}


static ssize_t nyx_qio_buffer_readv(QIOChannel *ioc,
                                        const struct iovec *iov,
                                        size_t niov,
                                        int **fds,
                                        size_t *nfds,
                                        int flags,
                                        Error **errp)
{
    NYXQIOBuffer *bioc = NYX_QIO_BUFFER(ioc);
    ssize_t ret = 0;
    size_t i;

    for (i = 0; i < niov; i++) {
        size_t want = iov[i].iov_len;
        if (bioc->opaque->pos >= bioc->opaque->buflen) {
            break;
        }
        if ((bioc->opaque->pos + want) > bioc->opaque->buflen)  {
            want = bioc->opaque->buflen - bioc->opaque->pos;
        }
        memcpy(iov[i].iov_base, bioc->opaque->buf + bioc->opaque->pos, want);
        ret += want;
        bioc->opaque->pos += want;
    }

    return ret;
}

static ssize_t nyx_qio_buffer_writev(QIOChannel *ioc,
                                         const struct iovec *iov,
                                         size_t niov,
                                         int *fds,
                                         size_t nfds,
                                         int flags,
                                         Error **errp)
{
    NYXQIOBuffer *bioc = NYX_QIO_BUFFER(ioc);
    ssize_t ret = 0;
    size_t i;

    // Pre-QIO-Channel NYX code didn't do any capacity checking for writes, so I guess I'll also omit it...

    for (i = 0; i < niov; i++) {
        memcpy((void *)(((struct fast_savevm_opaque_t *)(bioc->opaque))->buf +
                        ((struct fast_savevm_opaque_t *)(bioc->opaque))->pos),
               iov[i].iov_base, iov[i].iov_len);
        ((struct fast_savevm_opaque_t *)(bioc->opaque))->pos += iov[i].iov_len;
        ret += iov[i].iov_len;
    }

    return ret;
}

static int nyx_qio_buffer_set_blocking(QIOChannel *ioc G_GNUC_UNUSED,
                                           bool enabled G_GNUC_UNUSED,
                                           Error **errp G_GNUC_UNUSED)
{
    return 0;
}

static int nyx_qio_buffer_close(QIOChannel *ioc,
                                    Error **errp)
{
    // no need to free anything, this object doesn't manage memory

    return 0;
}


typedef struct NYXQIOBufferSource NYXQIOBufferSource;
struct NYXQIOBufferSource {
    GSource parent;
    NYXQIOBuffer *bioc;
    GIOCondition condition;
};

static gboolean
nyx_qio_buffer_source_prepare(GSource *source,
                                  gint *timeout)
{
    NYXQIOBufferSource *bsource = (NYXQIOBufferSource *)source;

    *timeout = -1;

    return (G_IO_IN | G_IO_OUT) & bsource->condition;
}

static gboolean
nyx_qio_buffer_source_check(GSource *source)
{
    NYXQIOBufferSource *bsource = (NYXQIOBufferSource *)source;

    return (G_IO_IN | G_IO_OUT) & bsource->condition;
}

static gboolean
nyx_qio_buffer_source_dispatch(GSource *source,
                                   GSourceFunc callback,
                                   gpointer user_data)
{
    QIOChannelFunc func = (QIOChannelFunc)callback;
    NYXQIOBufferSource *bsource = (NYXQIOBufferSource *)source;

    return (*func)(QIO_CHANNEL(bsource->bioc),
                   ((G_IO_IN | G_IO_OUT) & bsource->condition),
                   user_data);
}

static void
nyx_qio_buffer_source_finalize(GSource *source)
{
    NYXQIOBufferSource *ssource = (NYXQIOBufferSource *)source;

    object_unref(OBJECT(ssource->bioc));
}

GSourceFuncs nyx_qio_buffer_source_funcs = {
    nyx_qio_buffer_source_prepare,
    nyx_qio_buffer_source_check,
    nyx_qio_buffer_source_dispatch,
    nyx_qio_buffer_source_finalize
};

static GSource *nyx_qio_buffer_create_watch(QIOChannel *ioc,
                                                GIOCondition condition)
{
    NYXQIOBuffer *bioc = NYX_QIO_BUFFER(ioc);
    NYXQIOBufferSource *ssource;
    GSource *source;

    source = g_source_new(&nyx_qio_buffer_source_funcs,
                          sizeof(NYXQIOBufferSource));
    ssource = (NYXQIOBufferSource *)source;

    ssource->bioc = bioc;
    object_ref(OBJECT(bioc));

    ssource->condition = condition;

    return source;
}


static void nyx_qio_buffer_class_init(ObjectClass *klass,
                                          void *class_data G_GNUC_UNUSED)
{
    QIOChannelClass *ioc_klass = QIO_CHANNEL_CLASS(klass);

    ioc_klass->io_writev = nyx_qio_buffer_writev;
    ioc_klass->io_readv = nyx_qio_buffer_readv;
    ioc_klass->io_set_blocking = nyx_qio_buffer_set_blocking;
    ioc_klass->io_close = nyx_qio_buffer_close;
    ioc_klass->io_create_watch = nyx_qio_buffer_create_watch;
}

static const TypeInfo nyx_qio_buffer_info = {
    .parent = TYPE_QIO_CHANNEL,
    .name = TYPE_NYX_QIO_BUFFER,
    .instance_size = sizeof(NYXQIOBuffer),
    .instance_finalize = nyx_qio_buffer_finalize,
    .class_init = nyx_qio_buffer_class_init,
};

static void nyx_qio_buffer_register_types(void)
{
    type_register_static(&nyx_qio_buffer_info);
}

type_init(nyx_qio_buffer_register_types);