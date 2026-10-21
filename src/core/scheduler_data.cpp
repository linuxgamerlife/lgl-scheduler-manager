#include "core/scheduler_data.h"

namespace lsm {

const std::vector<std::string_view>& fallback_schedulers() {
    static const std::vector<std::string_view> schedulers{
        "bpfland", "beerland", "cake",        "cosmos", "flash",    "lavd",   "layered",
        "nest",    "p2dq",     "pandemonium", "rusty",  "rustland", "simple", "tickless"};
    return schedulers;
}

const std::vector<SchedulerInfo>& scheduler_reference() {
    static const std::vector<SchedulerInfo> entries{
        {"scx_bpfland", "General desktop, interactive apps, mixed workloads",
         "Pure batch/CPU-bound workloads where throughput matters more than latency",
         "Recommended default. Prioritises tasks that block frequently (interactive). "
         "Cache-topology aware - keeps tasks near their L2/L3 cache. Runs entirely in BPF "
         "(low overhead)."},
        {"scx_beerland", "General desktop, similar workloads to scx_bpfland but lower overhead",
         "Workloads needing the full feature set of bpfland",
         "A reduced-overhead variant of scx_bpfland by the same author. Strips back some of "
         "bpfland's more expensive per-task tracking to lower scheduler overhead on busy "
         "systems. Good alternative if bpfland feels heavy on your hardware."},
        {"scx_cake", "Experimental desktop/gaming use on CachyOS",
         "Production workloads - this is experimental",
         "CachyOS-specific experimental scheduler. Not yet in upstream scx. Intended for "
         "desktop and gaming workloads. Behaviour and flags may change significantly between "
         "updates - check the CachyOS Discord for current status."},
        {"scx_cosmos", "General desktop and server, mixed workloads, NUMA systems",
         "Workloads requiring strict real-time guarantees",
         "Lightweight locality-first scheduler. Keeps tasks on the same CPU using local DSQs "
         "when the system is not saturated - reduces cache misses and locking contention. Under "
         "saturation, switches to a deadline-based policy with a shared DSQ so interactive tasks "
         "can preempt CPU-bound ones. Uses 10µs time slices by default. Good general-purpose "
         "choice with low overhead."},
        {"scx_flash", "High-throughput batch jobs, compilation, video encoding, servers",
         "Latency-sensitive interactive use; gaming",
         "Optimised for throughput over responsiveness. Minimal overhead; suited to sustained "
         "CPU-bound workloads."},
        {"scx_lavd", "Gaming, audio production, real-time / low-latency workloads",
         "Battery-constrained devices (can increase power draw); pure server workloads",
         "Latency-Aware Virtual Deadline scheduler. Computes a latency-criticality score per "
         "task. Core Compaction keeps active cores at higher frequency when load < 50%, saving "
         "power. Autopilot mode auto-switches between performance/balanced/powersave."},
        {"scx_layered", "Power users who want per-application scheduling policies",
         "Users who don't want to write a config file",
         "Classifies threads into named layers (like cgroups) and applies a different policy to "
         "each. Highly flexible but requires manual JSON configuration."},
        {"scx_nest", "Systems where boosting clock speed matters; lightly-loaded desktops",
         "Heavily loaded systems (all cores busy)",
         "Places tasks on already-warm, high-frequency cores to keep turbo boost active. "
         "Effective when CPU utilisation is low to moderate."},
        {"scx_p2dq", "Mixed desktop/server, systems needing fair load balancing",
         "Workloads requiring strict real-time guarantees",
         "Pick-2 randomised load balancing keeps queues shallow. Simple design means low "
         "scheduler overhead. PELT-based load tracking."},
        {"scx_pandemonium", "Experimental use, testing dynamic task-behaviour learning",
         "Production workloads - this is experimental",
         "Written in Rust and C. Dynamically learns task behaviour over time to improve "
         "scheduling decisions. CachyOS experimental scheduler; not yet in upstream scx. May "
         "change significantly between updates."},
        {"scx_rusty", "Multi-core desktops and servers, NUMA systems, general-purpose",
         "Single-core or very low core-count machines",
         "Partitions CPUs by last-level cache domain to minimise cross-cache migration. Good "
         "scalability on high core-count systems. Hybrid BPF + userspace design."},
        {"scx_rustland", "Experimentation, userspace scheduler research",
         "Production use - higher latency than BPF-only schedulers",
         "Predecessor to scx_bpfland. Scheduling decisions run in userspace (Rust), which adds a "
         "context-switch overhead on every scheduling event. Interesting for learning how "
         "userspace schedulers work but not recommended for daily use."},
        {"scx_simple", "Testing, debugging, learning sched_ext, minimal systems",
         "Any production workload",
         "Minimal FIFO/least-runtime policy. No topology awareness. Useful as a baseline for "
         "benchmarking other schedulers."},
        {"scx_tickless", "Cloud computing, virtualisation, HPC, server batch workloads",
         "Latency-sensitive or interactive desktop use - nohz_full adds syscall overhead",
         "Routes all scheduling events through a small pool of primary CPUs, allowing other CPUs "
         "to run tickless (no scheduler interrupts). Reduces OS noise for VM guests and HPC jobs. "
         "Requires booting with nohz_full= kernel parameter to fully realise the benefit. Not "
         "designed for desktop or gaming use."},
    };
    return entries;
}

const std::vector<FlagInfo>& flag_reference() {
    static const std::vector<FlagInfo> entries{
        {"scx_bpfland", "-s / --slice-us", "<microseconds>",
         "Time slice duration in microseconds. Lower = more responsive. Default: 5000. Try "
         "500–2000 for low-latency gaming."},
        {"scx_bpfland", "-m / --sched-mode", "performance / powersave",
         "CPU frequency governor hint. 'performance' keeps clocks high; 'powersave' allows "
         "down-clocking."},
        {"scx_bpfland", "-w / --keep-running", "(flag)",
         "Prevents idle CPUs from being offlined. Useful on desktops to reduce wakeup latency."},
        {"scx_bpfland", "-k / --local-kthreads", "(flag)",
         "Pin kernel threads to their originating CPU's local DSQ. Can improve I/O throughput."},
        {"scx_bpfland", "-I / --idle-resume-us", "<microseconds>",
         "How long a CPU must be idle before it is considered for resume. Tune for power vs. "
         "latency trade-off."},
        {"scx_bpfland", "-t / --idle-cpus-pct", "<percent>",
         "Target percentage of idle CPUs to maintain. Higher = more power saving; lower = faster "
         "task dispatch."},
        {"scx_bpfland", "-S / --stop-once", "(flag)",
         "Stop the scheduler cleanly on the first error rather than restarting. Useful for "
         "debugging."},
        {"scx_lavd", "--performance", "(flag)",
         "Enable performance mode: disables power management and keeps all cores at high "
         "frequency. Best for gaming."},
        {"scx_lavd", "--powersave", "(flag)",
         "Enable powersave mode: aggressively packs tasks onto fewer cores, allows others to "
         "sleep."},
        {"scx_lavd", "--no-core-compaction", "(flag)",
         "Disable core compaction. Tasks spread freely across all cores rather than being "
         "consolidated."},
        {"scx_lavd", "--autopilot", "(flag)",
         "Let LAVD auto-switch between performance/balanced/powersave based on load. Overrides "
         "manual mode flags."},
        {"scx_lavd", "--slice-boost-max-x", "<multiplier>",
         "Maximum time slice boost multiplier for latency-critical tasks. Default: 10. Higher = "
         "more burst time for interactive tasks."},
        {"scx_rusty", "--slice-us", "<microseconds>",
         "Base time slice in microseconds. Default: 2000. Reduce for interactive work; increase "
         "for batch."},
        {"scx_rusty", "--interval", "<milliseconds>",
         "Scheduling decision interval. Lower = faster reaction to load changes but more "
         "overhead. Default: 100."},
        {"scx_rusty", "--tune-util", "<percent>",
         "Target CPU utilisation percentage per LLC domain. Helps balance load across cache "
         "domains on NUMA systems."},
        {"scx_rusty", "--greedy-idle-cpu", "(flag)",
         "Allow tasks to immediately steal idle CPUs outside their preferred LLC domain."},
        {"scx_flash", "-m / --sched-mode", "performance / powersave / all",
         "'performance' boosts interactive tasks; 'powersave' conserves energy; 'all' treats all "
         "tasks equally."},
        {"scx_flash", "-w / --keep-running", "(flag)",
         "Prevent CPUs from idling. Reduces wakeup latency at the cost of power."},
        {"scx_flash", "-C / --no-preempt", "0 / 1",
         "Disable preemption of running tasks by newly woken ones. 0 = allow preemption "
         "(default); 1 = disable."},
        {"scx_flash", "-s / --slice-us", "<microseconds>", "Base time slice. Default: 5000."},
        {"scx_flash", "-S / --slice-max-us", "<microseconds>",
         "Maximum allowed time slice. Default: 20000."},
        {"scx_flash", "-I / --idle-cpu-thresh", "<percent>",
         "Idle CPU threshold below which Flash stops trying to pack tasks."},
        {"scx_flash", "-D / --dispatch-order", "(flag)",
         "Honour task priority during dispatch rather than pure FIFO."},
        {"scx_flash", "-L / --llc-affinity", "(flag)",
         "Prefer keeping tasks within their LLC domain to maximise cache reuse."},
        {"scx_p2dq", "--task-slice true", "(bool)",
         "Enable per-task dynamic time slices instead of a fixed global slice. Improves fairness "
         "under mixed workloads."},
        {"scx_p2dq", "-f / --fifo-fallback", "(flag)",
         "Fall back to FIFO ordering within queues when load is low."},
        {"scx_p2dq", "--sched-mode", "performance",
         "Enable performance scheduling mode: reduced idle time, higher clock frequency hints."},
        {"scx_p2dq", "-y / --yield-preempt", "(flag)",
         "Allow tasks that call sched_yield() to preempt the current task immediately."},
        {"scx_p2dq", "--select-idle-in-enqueue", "(flag)",
         "Select an idle CPU at enqueue time rather than at dispatch. Can reduce latency on "
         "lightly loaded systems."},
        {"scx_cosmos", "-s / --slice-us", "<microseconds>",
         "Time slice duration. Default: 10µs - very short, designed to minimise latency. Increase "
         "(e.g. 5000) for throughput-heavy workloads."},
        {"scx_cosmos", "--numa", "(flag)",
         "Enable per-NUMA-node shared DSQs under saturation, instead of a single global DSQ. "
         "Improves locality on multi-socket systems."},
        {"scx_cosmos", "--stats", "<interval_s>",
         "Print scheduler statistics every N seconds. Useful for observing behaviour without "
         "scxtop."},
        {"scx_tickless", "--primary-domain", "<cpumask>",
         "Hex CPU mask for the primary scheduling CPUs. Default: only CPU 0. On large systems, "
         "e.g. --primary-domain 0xf assigns CPUs 0–3. More primaries = better task distribution "
         "but more overhead."},
        {"scx_tickless", "--frequency", "<hz>",
         "How often (in Hz) primary CPUs check tickless CPUs for contention. Lower = less "
         "overhead, higher = faster response to imbalance. Default: 1000."},
        {"scx_tickless", "--stats", "<interval_s>", "Print scheduling statistics every N seconds."},
        {"scx_layered", "--config / -f", "<path.json>",
         "Path to the JSON layer configuration file. Required - scx_layered does nothing useful "
         "without a config. See /usr/share/scx/examples/ for templates."},
        {"scx_layered", "--stats", "<interval_s>", "Print per-layer statistics every N seconds."},
        {"scx_layered", "--slice-us", "<microseconds>",
         "Base time slice for all layers. Individual layers can override this in their config."},
        {"scx_layered", "--exit-dump-len", "<lines>",
         "Number of lines of BPF ringbuf output to dump on exit. Useful for debugging layer "
         "misclassification."},
        {"scx_nest", "--primary-cpus", "<count>",
         "Number of 'primary' CPUs to keep warm at high frequency. Tasks preferentially run here "
         "when load is low."},
        {"scx_nest", "--slice-us", "<microseconds>", "Base time slice. Default: 20000."},
        {"scx_nest", "--r-imbalance", "<threshold>",
         "Load imbalance ratio that triggers task migration away from the primary nest. Lower = "
         "more aggressive rebalancing."},
        {"scx_rusty", "(none beyond basic)", "-",
         "scx_rusty exposes --slice-us, --interval, --tune-util, and --greedy-idle-cpu as noted "
         "above. All other behaviour is automatic."},
        {"scx_rustland", "(no flags)", "-",
         "scx_rustland has no user-facing configuration flags. All scheduling decisions are made "
         "automatically by the userspace Rust runtime."},
        {"scx_simple", "(no flags)", "-",
         "scx_simple is intentionally minimal with no configuration flags. Use it as-is for "
         "testing or benchmarking."},
        {"scx_beerland", "(no flags)", "-",
         "scx_beerland has no documented user-facing flags. It operates with the same defaults "
         "as scx_bpfland but with reduced overhead."},
        {"scx_pandemonium", "(no flags)", "-",
         "scx_pandemonium is a CachyOS experimental scheduler. No stable flags documented - run "
         "scx_pandemonium --help on your install to check current options."},
        {"scx_cake", "(no flags)", "-",
         "scx_cake is a CachyOS experimental scheduler. No stable flags documented - run "
         "scx_cake --help on your install to check current options."},
    };
    return entries;
}

} // namespace lsm
