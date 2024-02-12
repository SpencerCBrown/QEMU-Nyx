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

#ifndef NYX_QIO_BUFFER_H
#define NYX_QIO_BUFFER_H

#include "io/channel.h"
#include "qom/object.h"

#include "nyx/snapshot/devices/state_reallocation.h"

#define TYPE_NYX_QIO_BUFFER "nyx-qio-buffer"
OBJECT_DECLARE_SIMPLE_TYPE(NYXQIOBuffer, NYX_QIO_BUFFER)

/**
 * NYXQIOBuffer:
 *
 * The NYXQIOBuffer object provides a channel implementation
 * that is able to perform I/O to/from a memory buffer.
 * 
 * It differs from QIOChannelBuffer in that it allows the backing memory to be injected, 
 * rather than managed opaquely by the object.
 *
 */

struct NYXQIOBuffer {
    QIOChannel parent;
    struct fast_savevm_opaque_t* opaque;
    // size_t capacity; /* Total allocated memory */
    // size_t usage;    /* Current size of data */
    // size_t offset;   /* Offset for future I/O ops */
    // uint8_t *data;
};


/**
 * nyx_qio_buffer_new:
 * @opaque: the nyx data structure owning the backing memory buffers and metadata
 *
 * Allocate a new buffer which is initially empty
 *
 * Returns: the new channel object
 */
NYXQIOBuffer *
nyx_qio_buffer_new(struct fast_savevm_opaque_t* opaque);

#endif /* NYX_QIO_BUFFER_H */
