#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct i01_order_manager_t;
struct i01_data_manager_t;
struct i01_config_t;
struct i01_engine_app_t;
struct i01_strategy_t;
struct i01_named_thread_base_t;
typedef bool (*i01_register_strategy_op_t)(i01_strategy_t *);
typedef bool (*i01_register_named_thread_op_t)(i01_named_thread_base_t *);
/// Strategy plugin context, to allow plugins to create strategies more
/// easily.  This provides access to the configuration, order manager, etc.
/// Make sure to check .git_commit against your i01::g_COMMIT to verify that
/// your ABIs match!
/// The objects pointed to do not go out of scope when init exits.
extern "C" struct i01_StrategyPluginContext {
    i01_data_manager_t * data_manager;
    i01_order_manager_t * order_manager;
    i01_config_t * config_instance;
    i01_engine_app_t * engine_app;
    const char * git_commit;
    /// Register a Strategy with the engine using this function.
    i01_register_strategy_op_t register_strategy_op;
    /// Register a NamedThread with the engine using this function.
    /// You must register all threads or the engine will not join() on them
    /// prior to shutdown.  If you have any threads that are not NamedThreads,
    /// you should spawn them from a NamedThread (parent) and join() on them in that
    /// thread, then register the parent thread to prevent premature
    /// termination.
    i01_register_named_thread_op_t register_named_thread_op;
};

/// Type of function pointer to i01_strategy_plugin_init(), called on
/// initialization of the strategy.  Note that the strategy name (`name`) goes
/// out of scope after init returns, so you must copy the string if you want
/// to use it.  Returns false on failure, true on success.
typedef bool (*i01_StrategyPluginInitFunc)(const i01_StrategyPluginContext *, const char * cfg_name);
/// Type of function pointer to i01_strategy_plugin_exit(), called on close of
/// the engine or unloading of the strategy (only if gracefully shut down).
/// The strategy name (`name`) goes out of scope after exit returns.
typedef void (*i01_StrategyPluginExitFunc)(const i01_StrategyPluginContext *, const char * cfg_name);

#ifdef __cplusplus
}
#endif
