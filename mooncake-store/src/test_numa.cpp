#include <glog/logging.h>
#include <numa.h>
#include <thread>
#include <iostream>


void bind_to_numa_node(int node) {
    if (numa_available() < 0) {
        LOG(WARNING) << "NUMA is not available on this system; binding skipped";
        return;
    }

    int max_node = numa_max_node();
    if (node < 0 || node > max_node) {
        LOG(WARNING) << "Invalid NUMA node: " << node << ". Valid range: 0-"
                     << max_node;
    }

    // Bind current thread to CPUs of the NUMA node
    if (numa_run_on_node(node) != 0) {
        LOG(WARNING) << "numa_run_on_node failed for node " << node;
    }

    // Prefer this NUMA node for future allocations but allow fallback
    numa_set_bind_policy(0);  // non-strict binding
    numa_set_preferred(node);
}

void print_bind() {
    int node = numa_preferred();
    LOG(INFO) << "Preferred NUMA node: " << node;
    struct bitmask *run_node = numa_get_run_node_mask();
    LOG(INFO) << "Run NUMA node mask:";
    if (run_node) {
        LOG(INFO) << "  Size: " << run_node->size;
        LOG(INFO) << "  Bits: ";
        for (unsigned int i = 0; i < run_node->size; i++) {
            if (numa_bitmask_isbitset(run_node, i)) {
                LOG(INFO) << "    Node " << i << ": enabled";
            }
        }
    } else {
        LOG(INFO) << "  Run node mask is NULL";
    }
    int max_node = numa_max_node();
    LOG(INFO) << "Max NUMA node: " << max_node;
    int available = numa_available();
    LOG(INFO) << "numa_available: " << available;
    printf("=====================\n\n");
}

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;

    print_bind();
    printf("bind to numa node 0\n");
    bind_to_numa_node(0);
    print_bind();

    printf("bind to numa node 1\n");
    bind_to_numa_node(1);
    print_bind();

    // Create a new thread and call print_bind in it
    std::thread worker_thread([]() {
        printf("Thread: Calling print_bind from worker thread\n");
        print_bind();
    });

    // Wait for the thread to complete
    worker_thread.join();

    return 0;
}