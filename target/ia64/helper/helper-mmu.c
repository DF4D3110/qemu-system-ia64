/* IA-64 TCG helper ABI adapters for MMU serialization. */

#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/helper-proto.h"
#include "exec/tb-flush.h"
#include "arch/arch.h"
#include "trace.h"

void helper_tlb_serialize(CPUIA64State *env, uint32_t include_data,
                          uint32_t include_inst)
{
    ia64_tlb_serialize(env, include_data, include_inst);
}

void helper_merced_dtlb1_touch(CPUIA64State *env, uint64_t va, uint32_t size)
{
    uint64_t end = va + size - 1U;

    /*
     * Translated memory helpers normally revisit the same minimum page many
     * times before replacement.  Complete that direct-hit case here so the
     * generated helper call does not enter the general MMU access path.
     * Cross-page accesses and the theoretical LRU clock wrap retain the full
     * path, which touches both endpoints and rebases all ages respectively.
     */
    if (ia64_env_cpu_class(env)->model == IA64_CPU_MODEL_MERCED &&
        size != 0 && end >= va &&
        ia64_merced_dtlb1_lookup_page(end) ==
            ia64_merced_dtlb1_lookup_page(va) &&
        env->mmu.tlb_data_l1_clock != UINT64_MAX) {
        uint32_t rid = ia64_region_rid(env, va);
        int cached = ia64_merced_dtlb1_lookup(env, va, rid);

        if (cached >= 0) {
            env->mmu.tlb_data_l1_age[cached] =
                ++env->mmu.tlb_data_l1_clock;
            return;
        }
    }

    ia64_mmu_data_access(env, va, size, true);
}

void helper_fc(CPUIA64State *env, uint64_t addr)
{
    ia64_mmu_fc(env, addr);
}

uint32_t helper_sync_i(CPUIA64State *env)
{
    CPUState *cs = env_cpu(env);

    /*
     * fc.i performs precise invalidation as an optimization, but sync.i is
     * the architectural point at which all preceding instruction-cache
     * flushes become visible.  TCG's translated-code cache is global and can
     * contain aliases not represented by a single guest physical interval,
     * so conservatively complete the pending window with one global flush.
     * The generated code exits after this instruction, allowing the queued
     * operation to run from a safe CPU-loop context.
     */
    if (!env->mmu.icache_flush_pending &&
        !env->mmu.icache_sync_deferred) {
        return 0;
    }

    /*
     * An atomic helper can make TCG repeat an IA-64 bundle under the global
     * exclusive lock.  Flushing the code cache from target code in that
     * context would invalidate the TB that is still executing.  Complete the
     * architected instruction now and let the flagged next TB perform the
     * host-side flush after cpu_exec_step_atomic() releases the lock.
     */
    if (cpu_in_exclusive_context(cs)) {
        env->mmu.icache_sync_deferred = true;
        return 2;
    }

    env->mmu.icache_flush_pending = false;
    env->mmu.icache_sync_deferred = false;
    queue_tb_flush(cs);
    return 1;
}

void helper_sync_i_exit(CPUIA64State *env)
{
    cpu_loop_exit_noexc(env_cpu(env));
}

void helper_itr_insert(CPUIA64State *env, uint64_t pte, uint64_t slot_reg,
                       uint32_t is_data, uint64_t raw, uint32_t fault_slot)
{
    ia64_mmu_itr_insert(env, pte, slot_reg, is_data, raw, fault_slot);
}

void helper_ptr_purge(CPUIA64State *env, uint64_t ifa, uint64_t size_reg,
                      uint32_t is_data)
{
    ia64_mmu_ptr_purge(env, ifa, size_reg, is_data);
}

void helper_ptc_purge(CPUIA64State *env, uint64_t va, uint64_t size_reg,
                      uint32_t mode)
{
    ia64_mmu_ptc_purge(env, va, size_reg, mode);
}

uint64_t helper_tpa(CPUIA64State *env, uint64_t va)
{
    return ia64_mmu_tpa(env, va);
}

uint64_t helper_probe(CPUIA64State *env, uint64_t va, uint32_t is_write,
                      uint64_t access_level)
{
    return ia64_mmu_probe(env, va, is_write, access_level);
}

void helper_probe_fault(CPUIA64State *env, uint64_t va, uint32_t is_write,
                        uint32_t is_rw, uint64_t access_level)
{
    ia64_mmu_probe_fault(env, va, is_write, is_rw, access_level);
}

void helper_lfetch_fault(CPUIA64State *env, uint64_t va,
                         uint64_t fault_info, uint32_t hint)
{
    ia64_mmu_lfetch_fault(env, va, fault_info, hint);
}

void helper_check_semaphore_access(CPUIA64State *env, uint64_t va)
{
    ia64_mmu_check_semaphore_access(env, va);
}

void helper_check_montecito_16byte_access(CPUIA64State *env, uint64_t va,
                                          uint32_t is_write)
{
    ia64_mmu_check_montecito_16byte_access(env, va, is_write);
}

uint64_t helper_speculative_probe(CPUIA64State *env, uint64_t va,
                                  uint32_t is_write, uint32_t is_ifetch,
                                  uint32_t size)
{
    uint64_t result =
        ia64_mmu_speculative_probe(env, va, is_write, is_ifetch, size);

    if (!result) {
        trace_ia64_speculative_probe_defer(
            env_cpu(env)->cpu_index, env->ip, va, size);
    }
    return result;
}

uint64_t helper_advanced_load_allowed(CPUIA64State *env, uint64_t va)
{
    return ia64_mmu_advanced_load_allowed(env, va);
}

uint64_t helper_tak(CPUIA64State *env, uint64_t va)
{
    return ia64_mmu_tak(env, va);
}

uint64_t helper_thash(CPUIA64State *env, uint64_t va)
{
    return ia64_mmu_thash(env, va);
}

uint64_t helper_ttag(CPUIA64State *env, uint64_t va)
{
    return ia64_mmu_ttag(env, va);
}

void helper_itc_insert(CPUIA64State *env, uint64_t pte, uint32_t is_data,
                       uint64_t raw, uint32_t fault_slot)
{
    ia64_mmu_itc_insert(env, pte, is_data, raw, fault_slot);
}
