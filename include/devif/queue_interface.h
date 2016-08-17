/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef QUEUE_INTERFACE_H_
#define QUEUE_INTERFACE_H_ 1


#include <barrelfish/barrelfish.h>

#define MAX_DEVICE_NAME 256


#define ENDPOINT_TYPE_FORWARD 0x1
#define ENDPOINT_TYPE_BLOCK 0x2
#define ENDPOINT_TYPE_NET 0x3

// the user side of the queue
#define ENDPOINT_TYPE_USER 0xF

typedef uint32_t regionid_t;
typedef uint32_t bufferid_t;

struct devq;

struct devq_buf{
    regionid_t rid; // 4
    bufferid_t bid; // 8
    lpaddr_t addr; // 16
    size_t len; // 24
    uint64_t flags; // 32
};

/*
 * ===========================================================================
 * Backend function definitions
 * ===========================================================================
 */

// These functions must be implemented by the driver which is using the library
typedef errval_t (*devq_setup_t)(uint64_t *features, 
                                 bool* reconnect, char* name);
typedef errval_t (*devq_create_t)(struct devq *q, uint64_t flags);
typedef errval_t (*devq_destroy_t)(struct devq *q);
typedef errval_t (*devq_enqueue_t)(struct devq *q, regionid_t region_id,
                                   lpaddr_t base, size_t length,
                                   uint64_t misc_flags, bufferid_t* buffer_id);
typedef errval_t (*devq_dequeue_t)(struct devq *q, regionid_t* region_id,
                                   lpaddr_t* base, size_t* length,
                                   bufferid_t* buffer_id, uint64_t* misc_flags);
typedef errval_t (*devq_notify_t) (struct devq *q, uint8_t num_slots);
typedef errval_t (*devq_register_t)(struct devq *q, struct capref cap,
                                    regionid_t region_id);
typedef errval_t (*devq_deregister_t)(struct devq *q, regionid_t region_id);
typedef errval_t (*devq_control_t)(struct devq *q, uint64_t request,
                                   uint64_t value);

// The functions that the device driver has to export
struct devq_func_pointer {
    devq_setup_t setup;
    devq_create_t create;
    devq_destroy_t destroy;
    devq_register_t reg;
    devq_deregister_t dereg;
    devq_enqueue_t enq;
    devq_dequeue_t deq;
    devq_control_t ctrl;
    devq_notify_t notify;
};

// The devif device state
struct device_state {
    // device type
    uint8_t device_type;
    // name of the device
    char device_name[MAX_DEVICE_NAME];
    // pointer to device specific state
    void* q;
    // features of the device
    uint64_t features;
    // setup function pointer
    struct devq_func_pointer f;
    // bool to check export
    bool export_done;
};

 /*
 * ===========================================================================
 * Device queue interface export (for devices)
 * ===========================================================================
 */
 /**
  * @brief exports the devq interface so others (client side) can connect
  *
  * @param device_name   Device name that is exported to the other endpoints.
  *                      Required by the "user" side to connect to
  * @param features      The features the driver exports
  *
  * @returns error on failure or SYS_ERR_OK on success
  */

errval_t devq_driver_export(struct device_state* s);

/* ===========================================================================
 * Device queue creation and destruction
 * ===========================================================================
 */

 /**
  * @brief creates a queue 
  *
  * @param q             Return pointer to the devq (handle)
  * @param device_name   Device name of the device to which this queue belongs
  *                      (Driver itself is running in a separate process)
  * @param device_type   The type of the device
  * @param flags         Anything you can think of that makes sense for the device
  *                      and its driver?
  *
  * @returns error on failure or SYS_ERR_OK on success
  */

errval_t devq_create(struct devq **q,
                     char* device_name,
                     uint8_t device_type,
                     uint64_t flags);

 /**
  * @brief destroys the device queue
  *
  * @param q           The queue state to free (and the device queue to be 
                       shut down in the driver)
  *
  * @returns error on failure or SYS_ERR_OK on success
  */
errval_t devq_destroy(struct devq *q);


/*
 * ===========================================================================
 * Device specific state
 * ===========================================================================
 */

/**
 * @brief get the device specific state for a queue
 *
 * @param q           The device queue to get the state for
 *
 * @returns void pointer to the defice specific state
 */
void* devq_get_state(struct devq *q);

/**
 * @brief get the device specific state for a queue
 *
 * @param q           The device queue to set the state for
 * @param state       The state
 *
 * @returns void pointer to the defice specific state
 */
void devq_set_state(struct devq *q,
                    void* state);

/*
 * ===========================================================================
 * Datapath functions
 * ===========================================================================
 */
/*
 *
 * @brief enqueue a buffer into the device queue
 *
 * @param q             The device queue to call the operation on
 * @param region_id     Id of the memory region the buffer belongs to
 * @param base          Physical address of the start of the enqueued buffer
 * @param lenght        Lenght of the enqueued buffer
 * @param misc_flags    Any other argument that makes sense to the device queue
 * @param buffer_id     Return pointer to buffer id of the enqueued buffer 
 *                      buffer_id is assigned by the interface
 *
 * @returns error on failure or SYS_ERR_OK on success
 *
 */
errval_t devq_enqueue(struct devq *q,
                      regionid_t region_id,
                      lpaddr_t base,
                      size_t length,
                      uint64_t misc_flags,
                      bufferid_t* buffer_id);

/**
 * @brief dequeue a buffer from the device queue
 *
 * @param q             The device queue to call the operation on
 * @param region_id     Return pointer to the id of the memory 
 *                      region the buffer belongs to
 * @param base          Return pointer to the physical address of 
 *                      the of the buffer
 * @param lenght        Return pointer to the lenght of the dequeue buffer
 * @param buffer_id     Reutrn pointer to the buffer id of the dequeued buffer 
 * @param misc_flags    Return value from other endpoint
 *
 * @returns error on failure or SYS_ERR_OK on success
 *
 */

errval_t devq_dequeue(struct devq *q,
                      regionid_t* region_id,
                      lpaddr_t* base,
                      size_t* length,
                      bufferid_t* buffer_id,
                      uint64_t* misc_flags);

/*
 * ===========================================================================
 * Control Path
 * ===========================================================================
 */

/**
 * @brief Add a memory region that can be used as buffers to 
 *        the device queue
 *
 * @param q              The device queue to call the operation on
 * @param cap            A Capability for some memory
 * @param region_id      Return pointer to a region id that is assigned
 *                       to the memory
 *
 * @returns error on failure or SYS_ERR_OK on success
 *
 */
errval_t devq_register(struct devq *q,
                       struct capref cap,
                       regionid_t* region_id);

/**
 * @brief Remove a memory region 
 *
 * @param q              The device queue to call the operation on
 * @param region_id      The region id to remove from the device 
 *                       queues memory
 * @param cap            The capability to the removed memory
 *
 * @returns error on failure or SYS_ERR_OK on success
 *
 */
errval_t devq_deregister(struct devq *q,
                         regionid_t region_id,
                         struct capref* cap);

/**
 * @brief Send a notification about new buffers on the queue
 *
 * @param q      The device queue to call the operation on
 *
 * @returns error on failure or SYS_ERR_OK on success
 *
 */
errval_t devq_notify(struct devq *q);

/**
 * @brief Enforce coherency between of the buffers in the queue
 *        by either flushing the cache or invalidating it
 *
 * @param q      The device queue to call the operation on
 *
 * @returns error on failure or SYS_ERR_OK on success
 *
 */
errval_t devq_prepare(struct devq *q);

/**
 * @brief Send a control message to the device queue
 *
 * @param q          The device queue to call the operation on
 * @param request    The type of the control message*
 * @param value      The value for the request
 *
 * @returns error on failure or SYS_ERR_OK on success
 *
 */
errval_t devq_control(struct devq *q,
                      uint64_t request,
                      uint64_t value);

#endif /* QUEUE_INTERFACE_H_ */
