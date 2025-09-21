/**
 * @file distributed_management.h
 * @brief Distributed system management interface
 * @author DslsOS Team
 * @version 1.0
 * @date 2024
 */

#ifndef _DISTRIBUTED_MANAGEMENT_H_
#define _DISTRIBUTED_MANAGEMENT_H_

#include "dslos.h"
#include "kernel.h"

// Node ID type
typedef ULONG NODE_ID, *PNODE_ID;

// Cluster ID type
typedef ULONG CLUSTER_ID, *PCLUSTER_ID;

// Service ID type
typedef ULONG SERVICE_ID, *PSERVICE_ID;

// Load balancer ID type
typedef ULONG LOAD_BALANCER_ID, *PLOAD_BALANCER_ID;

// Message bus ID type
typedef ULONG MESSAGE_BUS_ID, *PMESSAGE_BUS_ID;

// Node types
typedef enum _NODE_TYPE {
    NODE_TYPE_WORKER,
    NODE_TYPE_MASTER,
    NODE_TYPE_COORDINATOR,
    NODE_TYPE_GATEWAY,
    NODE_TYPE_STORAGE,
    NODE_TYPE_COMPUTE,
    NODE_TYPE_NETWORK
} NODE_TYPE, *PNODE_TYPE;

// Node states
typedef enum _NODE_STATE {
    NODE_STATE_OFFLINE,
    NODE_STATE_JOINING,
    NODE_STATE_ONLINE,
    NODE_STATE_BUSY,
    NODE_STATE_MAINTENANCE,
    NODE_STATE_DRAINING,
    NODE_STATE_LEAVING,
    NODE_STATE_ERROR
} NODE_STATE, *PNODE_STATE;

// Cluster states
typedef enum _CLUSTER_STATE {
    CLUSTER_STATE_INITIALIZING,
    CLUSTER_STATE_ACTIVE,
    CLUSTER_STATE_DEGRADED,
    CLUSTER_STATE_MAINTENANCE,
    CLUSTER_STATE_TERMINATED
} CLUSTER_STATE, *PCLUSTER_STATE;

// Service states
typedef enum _SERVICE_STATE {
    SERVICE_STATE_CREATED,
    SERVICE_STATE_STARTING,
    SERVICE_STATE_RUNNING,
    SERVICE_STATE_STOPPING,
    SERVICE_STATE_STOPPED,
    SERVICE_STATE_ERROR,
    SERVICE_STATE_UPDATING,
    SERVICE_STATE_SCALING
} SERVICE_STATE, *PSERVICE_STATE;

// Load balancer states
typedef enum _LOAD_BALANCER_STATE {
    LOAD_BALANCER_STATE_CREATED,
    LOAD_BALANCER_STATE_ACTIVE,
    LOAD_BALANCER_STATE_DRAINING,
    LOAD_BALANCER_STATE_STOPPED,
    LOAD_BALANCER_STATE_ERROR
} LOAD_BALANCER_STATE, *PLOAD_BALANCER_STATE;

// Message bus states
typedef enum _MESSAGE_BUS_STATE {
    MESSAGE_BUS_STATE_CREATED,
    MESSAGE_BUS_STATE_ACTIVE,
    MESSAGE_BUS_STATE_STOPPED,
    MESSAGE_BUS_STATE_ERROR
} MESSAGE_BUS_STATE, *PMESSAGE_BUS_STATE;

// Node capabilities
#define NODE_CAP_COMPUTE         0x00000001
#define NODE_CAP_STORAGE         0x00000002
#define NODE_CAP_NETWORK         0x00000004
#define NODE_CAP_GPU             0x00000008
#define NODE_CAP_ACCELERATOR     0x00000010
#define NODE_CAP_CONTAINER       0x00000020
#define NODE_CAP_VIRTUALIZATION  0x00000040
#define NODE_CAP_SECURITY        0x00000080

// Load balancing algorithms
typedef enum _LOAD_BALANCING_ALGORITHM {
    LOAD_BALANCING_ROUND_ROBIN,
    LOAD_BALANCING_LEAST_CONNECTIONS,
    LOAD_BALANCING_IP_HASH,
    LOAD_BALANCING_WEIGHTED_ROUND_ROBIN,
    LOAD_BALANCING_WEIGHTED_LEAST_CONNECTIONS,
    LOAD_BALANCING_RANDOM
} LOAD_BALANCING_ALGORITHM, *PLOAD_BALANCING_ALGORITHM;

// Cluster configuration
typedef struct _CLUSTER_CONFIG {
    ULONG MaxNodes;
    ULONG ReplicationFactor;
    ULONG ConsistencyLevel;
    ULONG PartitionStrategy;
    BOOLEAN AutoFailover;
    ULONG FailoverTimeout;
    ULONG HealthCheckInterval;
    ULONG QuorumRequirement;
    UNICODE_STRING NetworkAddress;
    USHORT Port;
} CLUSTER_CONFIG, *PCLUSTER_CONFIG;

// Service configuration
typedef struct _SERVICE_CONFIG {
    UNICODE_STRING ConfigPath;
    UNICODE_STRING WorkingDirectory;
    UNICODE_STRING ExecutablePath;
    UNICODE_STRING Arguments;
    UNICODE_STRING Environment;
    ULONG Replicas;
    ULONG DeploymentStrategy;
    ULONG UpdateStrategy;
    ULONG MinReplicas;
    ULONG MaxReplicas;
    ULONG TargetCpuUsage;
    ULONG TargetMemoryUsage;
    ULONG HealthCheckInterval;
    UNICODE_STRING HealthCheckEndpoint;
    RESOURCE_REQUIREMENTS Requirements;
    RESOURCE_LIMITS Limits;
    SERVICE_SECURITY Security;
} SERVICE_CONFIG, *PSERVICE_CONFIG;

// Node information
typedef struct _NODE_INFO {
    NODE_ID NodeId;
    UNICODE_STRING NodeName;
    UNICODE_STRING NodeAddress;
    NODE_TYPE NodeType;
    NODE_STATE State;
    UNICODE_STRING IpAddress;
    USHORT Port;
    UNICODE_STRING MacAddress;
    ULONG CpuCount;
    ULONG64 TotalMemory;
    ULONG64 AvailableMemory;
    ULONG64 TotalStorage;
    ULONG64 AvailableStorage;
    ULONG Capabilities;
    ULONG AllocatedCpu;
    ULONG64 AllocatedMemory;
    ULONG64 AllocatedStorage;
    ULONG CpuUsage;
    ULONG MemoryUsage;
    ULONG NetworkUsage;
    ULONG DiskUsage;
    ULONG LoadAverage;
    ULONG Temperature;
    BOOLEAN IsConnected;
    LARGE_INTEGER LastHeartbeat;
    ULONG MissedHeartbeats;
    ULONG Latency;
    BOOLEAN InMaintenance;
    LARGE_INTEGER MaintenanceStart;
    UNICODE_STRING MaintenanceReason;
    ULONG HealthScore;
    ULONG ErrorCount;
    ULONG WarningCount;
    ULONG RecoveryCount;
    CLUSTER_ID ClusterId;
    NODE_ID MasterNodeId;
    BOOLEAN IsMaster;
    ULONG NodeRank;
    ULONG CurrentLoad;
    ULONG MaxLoad;
    ULONG LoadFactor;
    BOOLEAN IsHealthy;
    BOOLEAN FailoverEnabled;
    NODE_ID FailoverPartner;
    LARGE_INTEGER JoinTime;
    LARGE_INTEGER LastUpdate;
} NODE_INFO, *PNODE_INFO;

// Cluster information
typedef struct _CLUSTER_INFORMATION {
    CLUSTER_ID ClusterId;
    CLUSTER_STATE State;
    UNICODE_STRING ClusterName;
    UNICODE_STRING ClusterDescription;
    ULONG NodeCount;
    ULONG OnlineNodes;
    ULONG OfflineNodes;
    ULONG BusyNodes;
    ULONG ServiceCount;
    NODE_ID MasterNodeId;
    CLUSTER_RESOURCES Resources;
    CLUSTER_ALLOCATION Allocation;
    CLUSTER_METRICS Metrics;
    CLUSTER_HEALTH Health;
} CLUSTER_INFORMATION, *PCLUSTER_INFORMATION;

// Service information
typedef struct _SERVICE_INFORMATION {
    SERVICE_ID ServiceId;
    SERVICE_STATE State;
    UNICODE_STRING ServiceName;
    UNICODE_STRING ServiceType;
    CLUSTER_ID ClusterId;
    NODE_ID PrimaryNodeId;
    SERVICE_DEPLOYMENT Deployment;
    SERVICE_SCALING Scaling;
    RESOURCE_REQUIREMENTS Requirements;
    RESOURCE_LIMITS Limits;
    SERVICE_HEALTH Health;
    SERVICE_METRICS Metrics;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER LastActivity;
} SERVICE_INFORMATION, *PSERVICE_INFORMATION;

// Distributed system statistics
typedef struct _DISTRIBUTED_SYSTEM_STATS {
    ULONG TotalClusters;
    ULONG ActiveClusters;
    ULONG TotalNodes;
    ULONG OnlineNodes;
    ULONG TotalServices;
    ULONG RunningServices;
} DISTRIBUTED_SYSTEM_STATS, *PDISTRIBUTED_SYSTEM_STATS;

// Function prototypes

// Distributed system initialization
NTSTATUS
NTAPI
DmInitializeDistributedSystem(VOID);

// Cluster management
NTSTATUS
NTAPI
DmCreateCluster(
    _In_ PCWSTR ClusterName,
    _In_ PCWSTR ClusterDescription,
    _In_ PCLUSTER_CONFIG Config,
    _Out_ PCLUSTER_ID ClusterId
);

NTSTATUS
NTAPI
DmJoinCluster(
    _In_ CLUSTER_ID ClusterId,
    _In_ PCWSTR NodeAddress
);

NTSTATUS
NTAPI
DmLeaveCluster(VOID);

NTSTATUS
NTAPI
DmGetClusterInfo(
    _In_ CLUSTER_ID ClusterId,
    _Out_ PCLUSTER_INFORMATION Info
);

PCLUSTER_INFO
NTAPI
DmFindClusterById(
    _In_ CLUSTER_ID ClusterId
);

// Node management
NTSTATUS
NTAPI
DmSendHeartbeat(
    _In_ NODE_ID NodeId
);

PNODE_INFO
NTAPI
DmFindNodeById(
    _In_ NODE_ID NodeId
);

// Service management
NTSTATUS
NTAPI
DmCreateService(
    _In_ PCWSTR ServiceName,
    _In_ PCWSTR ServiceType,
    _In_ PSERVICE_CONFIG Config,
    _Out_ PSERVICE_ID ServiceId
);

NTSTATUS
NTAPI
DmStartService(
    _In_ SERVICE_ID ServiceId
);

NTSTATUS
NTAPI
DmStopService(
    _In_ SERVICE_ID ServiceId,
    _In_ BOOLEAN Force
);

NTSTATUS
NTAPI
DmScaleService(
    _In_ SERVICE_ID ServiceId,
    _In_ ULONG Replicas
);

NTSTATUS
NTAPI
DmGetServiceInfo(
    _In_ SERVICE_ID ServiceId,
    _Out_ PSERVICE_INFORMATION Info
);

PSERVICE_INFO
NTAPI
DmFindServiceById(
    _In_ SERVICE_ID ServiceId
);

// Statistics
NTSTATUS
NTAPI
DmGetDistributedSystemStatistics(
    _Out_ PDISTRIBUTED_SYSTEM_STATS Stats
);

// Status
BOOLEAN
NTAPI
DmIsDistributedSystemInitialized(VOID);

#endif // _DISTRIBUTED_MANAGEMENT_H_