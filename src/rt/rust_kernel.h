// -*- c++ -*-
#ifndef RUST_KERNEL_H
#define RUST_KERNEL_H

#include "rust_globals.h"

#include <map>
#include <vector>

#include "memory_region.h"
#include "rust_log.h"
#include "rust_sched_reaper.h"
#include "util/hash_map.h"

struct rust_task_thread;
class rust_scheduler;
class rust_port;

typedef intptr_t rust_sched_id;
typedef intptr_t rust_task_id;
typedef intptr_t rust_port_id;

typedef std::map<rust_sched_id, rust_scheduler*> sched_map;

class rust_sched_driver;
class rust_sched_launcher_factory;

/**
 * A global object shared by all thread domains. Most of the data structures
 * in this class are synchronized since they are accessed from multiple
 * threads.
 */
class rust_kernel {
    memory_region _region;
    rust_log _log;

    // The next task id
    rust_task_id max_task_id;

    // Protects max_port_id and port_table
    lock_and_signal port_lock;
    // The next port id
    rust_task_id max_port_id;
    hash_map<rust_port_id, rust_port *> port_table;

    lock_and_signal rval_lock;
    int rval;

    // Protects max_sched_id and sched_table, join_list
    lock_and_signal sched_lock;
    // The next scheduler id
    rust_sched_id max_sched_id;
    // A map from scheduler ids to schedulers. When this is empty
    // the kernel terminates
    sched_map sched_table;
    // A list of scheduler ids that are ready to exit
    std::vector<rust_sched_id> join_list;

    rust_sched_reaper sched_reaper;
    // The single-threaded scheduler that uses the main thread
    rust_sched_id osmain_scheduler;
    // Runs the single-threaded scheduler that executes tasks
    // on the main thread
    rust_sched_driver *osmain_driver;

    // An atomically updated count of the live, 'non-weak' tasks
    uintptr_t non_weak_tasks;
    // Protects weak_task_chans
    lock_and_signal weak_task_lock;
    // A list of weak tasks that need to be told when to exit
    std::vector<rust_port_id> weak_task_chans;

    rust_scheduler* get_scheduler_by_id_nolock(rust_sched_id id);
    void end_weak_tasks();

    // Used to communicate with the process-side, global libuv loop
    uintptr_t global_loop_chan;
    uintptr_t async_handle_inner;
    uintptr_t* global_async_handle;

public:
    struct rust_env *env;

    rust_kernel(rust_env *env);

    void log(uint32_t level, char const *fmt, ...);
    void fatal(char const *fmt, ...);

    void *malloc(size_t size, const char *tag);
    void *realloc(void *mem, size_t size);
    void free(void *mem);
    memory_region *region() { return &_region; }

    void fail();

    rust_sched_id create_scheduler(size_t num_threads);
    rust_sched_id create_scheduler(rust_sched_launcher_factory *launchfac,
                                   size_t num_threads, bool allow_exit);
    rust_scheduler* get_scheduler_by_id(rust_sched_id id);
    // Called by a scheduler to indicate that it is terminating
    void release_scheduler_id(rust_sched_id id);
    void wait_for_schedulers();
    int run();

#ifdef __WIN32__
    void win32_require(LPCTSTR fn, BOOL ok);
#endif

    rust_task_id generate_task_id();

    rust_port_id register_port(rust_port *port);
    rust_port *get_port_by_id(rust_port_id id);
    void release_port_id(rust_port_id tid);

    void set_exit_status(int code);

    rust_sched_id osmain_sched_id() { return osmain_scheduler; }

    void register_task();
    void unregister_task();
    void weaken_task(rust_port_id chan);
    void unweaken_task(rust_port_id chan);

    bool send_to_port(rust_port_id chan, void *sptr);

    uintptr_t* get_global_loop() { return &global_loop_chan; }
    uintptr_t* get_global_async_handle() { return global_async_handle; }
};

template <typename T> struct kernel_owned {
    inline void *operator new(size_t size, rust_kernel *kernel,
                              const char *tag) {
        return kernel->malloc(size, tag);
    }

    void operator delete(void *ptr) {
        ((T *)ptr)->kernel->free(ptr);
    }
};

#endif /* RUST_KERNEL_H */
