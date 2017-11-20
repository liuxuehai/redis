#ifndef __CLUSTER_H
#define __CLUSTER_H

/*-----------------------------------------------------------------------------
 * Redis cluster data structures, defines, exported API.
 * Redis集群数据结构定义，发布API。
 *----------------------------------------------------------------------------*/

#define CLUSTER_SLOTS 16384
#define CLUSTER_OK 0          /* Everything looks ok  一切看起来都好*/
#define CLUSTER_FAIL 1        /* The cluster can't work 群集不能工作 */
#define CLUSTER_NAMELEN 40    /* sha1 hex length SHA116进制长度*/
#define CLUSTER_PORT_INCR 10000 /* Cluster port = baseport + PORT_INCR 集群端口=基本港口+ port_incr*/

/* The following defines are amount of time, sometimes expressed as
 * multiplicators of the node timeout value (when ending with MULT).
 * 下面的定义是时间，有时表示为节点超时值（当以MULT结束）。*/
#define CLUSTER_DEFAULT_NODE_TIMEOUT 15000
#define CLUSTER_DEFAULT_SLAVE_VALIDITY 10 /* Slave max data age factor.  从数据文件的行为因素*/
#define CLUSTER_DEFAULT_REQUIRE_FULL_COVERAGE 1
#define CLUSTER_FAIL_REPORT_VALIDITY_MULT 2 /* Fail report validity.  失效报告有效性。*/
#define CLUSTER_FAIL_UNDO_TIME_MULT 2 /* Undo fail if master is back.  如果主节点回来了，撤消失败*/
#define CLUSTER_FAIL_UNDO_TIME_ADD 10 /* Some additional time.  一些额外的时间*/
#define CLUSTER_FAILOVER_DELAY 5 /* Seconds 秒*/
#define CLUSTER_DEFAULT_MIGRATION_BARRIER 1
#define CLUSTER_MF_TIMEOUT 5000 /* Milliseconds to do a manual failover.  进行手动故障转移的毫秒时间。*/
#define CLUSTER_MF_PAUSE_MULT 2 /* Master pause manual failover mult.  主节点暂停手动故障转移的多。*/
#define CLUSTER_SLAVE_MIGRATION_DELAY 5000 /* Delay for slave migration.  从节点迁移延迟*/

/* Redirection errors returned by getNodeByQuery().  重定向错误通过getNodeByQuery()。*/
#define CLUSTER_REDIR_NONE 0          /* Node can serve the request. 节点可以为请求服务 */
#define CLUSTER_REDIR_CROSS_SLOT 1    /* -CROSSSLOT request. -CROSSSLOT请求 */
#define CLUSTER_REDIR_UNSTABLE 2      /* -TRYAGAIN redirection required  -TRYAGAIN重定向需要*/
#define CLUSTER_REDIR_ASK 3           /* -ASK redirection required.  -ASK重定向需要*/
#define CLUSTER_REDIR_MOVED 4         /* -MOVED redirection required. -MOVED重定向需要*/
#define CLUSTER_REDIR_DOWN_STATE 5    /* -CLUSTERDOWN, global state.  -CLUSTERDOWN 全局状态*/
#define CLUSTER_REDIR_DOWN_UNBOUND 6  /* -CLUSTERDOWN, unbound slot.  -CLUSTERDOWN 无界缝隙*/

struct clusterNode;

/* clusterLink encapsulates everything needed to talk with a remote node.
 * clusterLink封装需要与远程节点都谈。 */
typedef struct clusterLink {
    mstime_t ctime;             /* Link creation time  链接创建时间*/
    int fd;                     /* TCP socket file descriptor  TCP 套接字描述*/
    sds sndbuf;                 /* Packet send buffer  发送的缓存数据包*/
    sds rcvbuf;                 /* Packet reception buffer  接收的缓存数据包*/
    struct clusterNode *node;   /* Node related to this link if any, or NULL  关联到这个 链接的node*/
} clusterLink;

/* Cluster node flags and macros.  集群节点标志和宏。*/
#define CLUSTER_NODE_MASTER 1     /* The node is a master  主节点*/
#define CLUSTER_NODE_SLAVE 2      /* The node is a slave  从节点*/
#define CLUSTER_NODE_PFAIL 4      /* Failure? Need acknowledge  失败了,需要通知*/
#define CLUSTER_NODE_FAIL 8       /* The node is believed to be malfunctioning  该节点被认为存在故障。*/
#define CLUSTER_NODE_MYSELF 16    /* This node is myself  自己节点*/
#define CLUSTER_NODE_HANDSHAKE 32 /* We have still to exchange the first ping  我们还得交换第一个ping。*/
#define CLUSTER_NODE_NOADDR   64  /* We don't know the address of this node  我们不知道这个节点的地址。*/
#define CLUSTER_NODE_MEET 128     /* Send a MEET message to this node  向这个节点发送一条MEET消息*/
#define CLUSTER_NODE_MIGRATE_TO 256 /* Master elegible for replica migration. 主节点elegible副本迁移*/
#define CLUSTER_NODE_NULL_NAME "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"

#define nodeIsMaster(n) ((n)->flags & CLUSTER_NODE_MASTER)
#define nodeIsSlave(n) ((n)->flags & CLUSTER_NODE_SLAVE)
#define nodeInHandshake(n) ((n)->flags & CLUSTER_NODE_HANDSHAKE)
#define nodeHasAddr(n) (!((n)->flags & CLUSTER_NODE_NOADDR))
#define nodeWithoutAddr(n) ((n)->flags & CLUSTER_NODE_NOADDR)
#define nodeTimedOut(n) ((n)->flags & CLUSTER_NODE_PFAIL)
#define nodeFailed(n) ((n)->flags & CLUSTER_NODE_FAIL)

/* Reasons why a slave is not able to failover.  一个从节点不能故障转移的原因。*/
#define CLUSTER_CANT_FAILOVER_NONE 0
#define CLUSTER_CANT_FAILOVER_DATA_AGE 1
#define CLUSTER_CANT_FAILOVER_WAITING_DELAY 2
#define CLUSTER_CANT_FAILOVER_EXPIRED 3
#define CLUSTER_CANT_FAILOVER_WAITING_VOTES 4
#define CLUSTER_CANT_FAILOVER_RELOG_PERIOD (60*5) /* seconds. */

/* This structure represent elements of node->fail_reports.
 * 这种结构的代表元素节点-> fail_reports。*/
typedef struct clusterNodeFailReport {
    struct clusterNode *node;  /* Node reporting the failure condition.  报告故障情况的节点*/
    mstime_t time;             /* Time of the last report from this node.  从这个节点上一次报告的时间*/
} clusterNodeFailReport;

typedef struct clusterNode {
    mstime_t ctime; /* Node object creation time. 节点创建时间 */
    char name[CLUSTER_NAMELEN]; /* Node name, hex string, sha1-size  节点名称*/
    int flags;      /* CLUSTER_NODE_... */
    uint64_t configEpoch; /* Last configEpoch observed for this node 最后configEpoch观察这个节点*/
    unsigned char slots[CLUSTER_SLOTS/8]; /* slots handled by this node */
    int numslots;   /* Number of slots handled by this node  此节点处理的槽数*/
    int numslaves;  /* Number of slave nodes, if this is a master  如果是主节点，则从节点数*/
    struct clusterNode **slaves; /* pointers to slave nodes  指向从节点的指针*/
    struct clusterNode *slaveof; /* pointer to the master node. Note that it
                                    may be NULL even if the node is a slave
                                    if we don't have the master node in our
                                    tables.  指向主节点的指针。注意它即使节点是从属的，也可以是空的。如果我们没有主节点.*/
    mstime_t ping_sent;      /* Unix time we sent latest ping  最近一次发送ping的时间*/
    mstime_t pong_received;  /* Unix time we received the pong  最近一次获取pong的时间*/
    mstime_t fail_time;      /* Unix time when FAIL flag was set  FAIL标记设置的时间*/
    mstime_t voted_time;     /* Last time we voted for a slave of this master  最近一次选举时间*/
    mstime_t repl_offset_time;  /* Unix time we received offset for this node  UNIX时间，我们收到这个节点的偏移量*/
    mstime_t orphaned_time;     /* Starting time of orphaned master condition  孤立主状态的开始时间*/
    long long repl_offset;      /* Last known repl offset for this node. 此节点的最后已知REPL偏移*/
    char ip[NET_IP_STR_LEN];  /* Latest known IP address of this node  此节点最近已知的IP地址*/
    int port;                   /* Latest known port of this node  此节点的最新已知端口*/
    clusterLink *link;          /* TCP/IP link with this node  与此节点的TCP/IP链路*/
    list *fail_reports;         /* List of nodes signaling this as failing  表示失败的节点列表*/
} clusterNode;

typedef struct clusterState {
    clusterNode *myself;  /* This node 这个节点 */
    uint64_t currentEpoch;
    int state;            /* CLUSTER_OK, CLUSTER_FAIL, ... */
    int size;             /* Num of master nodes with at least one slot  主节点至少有一个槽数*/
    dict *nodes;          /* Hash table of name -> clusterNode structures  哈希表名称-> clusterNode结构*/
    dict *nodes_black_list; /* Nodes we don't re-add for a few seconds.  我们不会再添加几秒钟的节点。*/
    clusterNode *migrating_slots_to[CLUSTER_SLOTS];
    clusterNode *importing_slots_from[CLUSTER_SLOTS];
    clusterNode *slots[CLUSTER_SLOTS];
    zskiplist *slots_to_keys;
    /* The following fields are used to take the slave state on elections.
     * 以下字段用于在选举中使用从属状态*/
    mstime_t failover_auth_time; /* Time of previous or next election. 上一次或下次选举时间*/
    int failover_auth_count;    /* Number of votes received so far. 到目前为止收到的选举数*/
    int failover_auth_sent;     /* True if we already asked for votes. 如果已经被请求选举为true*/
    int failover_auth_rank;     /* This slave rank for current auth request. 从节点等级为当前认证请求*/
    uint64_t failover_auth_epoch; /* Epoch of the current election. 当前选举的Epoch。*/
    int cant_failover_reason;   /* Why a slave is currently not able to
                                   failover. See the CANT_FAILOVER_* macros.为什么奴隶现在不能故障转移。 */
    /* Manual failover state in common.  常见的手动故障转移状态*/
    mstime_t mf_end;            /* Manual failover time limit (ms unixtime).
                                   It is zero if there is no MF in progress.
                                          手动故障转移的时间限制（MS unixtime）。如果没有MF，则为零。*/
    /* Manual failover state of master.  主的手动故障转移状态*/
    clusterNode *mf_slave;      /* Slave performing the manual failover. 执行手动故障转移的从节点*/
    /* Manual failover state of slave.  从属的手动故障转移状态。*/
    long long mf_master_offset; /* Master offset the slave needs to start MF
                                   or zero if stil not received.  主偏移奴隶需要如果还没有收到开始MF或零*/
    int mf_can_start;           /* If non-zero signal that the manual failover
                                   can start requesting masters vote.  如果非零信号，手动故障转移可以开始请求主投票。*/
    /* The followign fields are used by masters to take state on elections.
     * 换算字段用于主节点把选举成功*/
    uint64_t lastVoteEpoch;     /* Epoch of the last vote granted.  最后一次投票的Epoch。*/
    int todo_before_sleep; /* Things to do in clusterBeforeSleep(). clusterBeforeSleep()要处理的事*/
    long long stats_bus_messages_sent;  /* Num of msg sent via cluster bus.  通过集群发送的消息*/
    long long stats_bus_messages_received; /* Num of msg rcvd via cluster bus. 通过集群接收的消息*/
} clusterState;

/* clusterState todo_before_sleep flags. */
#define CLUSTER_TODO_HANDLE_FAILOVER (1<<0)
#define CLUSTER_TODO_UPDATE_STATE (1<<1)
#define CLUSTER_TODO_SAVE_CONFIG (1<<2)
#define CLUSTER_TODO_FSYNC_CONFIG (1<<3)

/* Redis cluster messages header  Redis集群消息头 */

/* Note that the PING, PONG and MEET messages are actually the same exact
 * kind of packet. PONG is the reply to ping, in the exact format as a PING,
 * while MEET is a special PING that forces the receiver to add the sender
 * as a node (if it is not already in the list).
 * 请注意，ping、PONG和MEET消息实际上是相同的包类型。
 * PONG是ping的应答，以PING的确切格式，而MEET是一种特殊的ping，强制接收方将发送方作为节点（如果它不在列表中）。*/
#define CLUSTERMSG_TYPE_PING 0          /* Ping */
#define CLUSTERMSG_TYPE_PONG 1          /* Pong (reply to Ping) */
#define CLUSTERMSG_TYPE_MEET 2          /* Meet "let's join" message */
#define CLUSTERMSG_TYPE_FAIL 3          /* Mark node xxx as failing  标记节点失败*/
#define CLUSTERMSG_TYPE_PUBLISH 4       /* Pub/Sub Publish propagation 发布/获取 传播*/
#define CLUSTERMSG_TYPE_FAILOVER_AUTH_REQUEST 5 /* May I failover?  可以失败吗*/
#define CLUSTERMSG_TYPE_FAILOVER_AUTH_ACK 6     /* Yes, you have my vote  可以失败的确认*/
#define CLUSTERMSG_TYPE_UPDATE 7        /* Another node slots configuration  另一个节点槽配置*/
#define CLUSTERMSG_TYPE_MFSTART 8       /* Pause clients for manual failover  暂停客户端进行手动故障转移*/

/* Initially we don't know our "name", but we'll find it once we connect
 * to the first node, using the getsockname() function. Then we'll use this
 * address for all the next messages.
 * 最初我们不知道我们的"name"，但我们会发现当我们连接到第一个节点，使用getsockname()功能。然后我们将为所有的下一个消息使用这个地址。*/
typedef struct {
    char nodename[CLUSTER_NAMELEN];
    uint32_t ping_sent;
    uint32_t pong_received;
    char ip[NET_IP_STR_LEN];  /* IP address last time it was seen */
    uint16_t port;              /* port last time it was seen */
    uint16_t flags;             /* node->flags copy */
    uint16_t notused1;          /* Some room for future improvements. */
    uint32_t notused2;
} clusterMsgDataGossip;

typedef struct {
    char nodename[CLUSTER_NAMELEN];
} clusterMsgDataFail;

typedef struct {
    uint32_t channel_len;
    uint32_t message_len;
    /* We can't reclare bulk_data as bulk_data[] since this structure is
     * nested. The 8 bytes are removed from the count during the message
     * length computation.
     * 我们不能reclare bulk_data作为bulk_data [ ]由于本结构嵌套。在消息长度计算期间，从计数中删除8字节。*/
    unsigned char bulk_data[8];
} clusterMsgDataPublish;

typedef struct {
    uint64_t configEpoch; /* Config epoch of the specified instance. 指定实例的配置epoch */
    char nodename[CLUSTER_NAMELEN]; /* Name of the slots owner. slots的拥有者的名称*/
    unsigned char slots[CLUSTER_SLOTS/8]; /* Slots bitmap. slots的位图*/
} clusterMsgDataUpdate;

union clusterMsgData {
    /* PING, MEET and PONG */
    struct {
        /* Array of N clusterMsgDataGossip structures */
        clusterMsgDataGossip gossip[1];
    } ping;

    /* FAIL */
    struct {
        clusterMsgDataFail about;
    } fail;

    /* PUBLISH */
    struct {
        clusterMsgDataPublish msg;
    } publish;

    /* UPDATE */
    struct {
        clusterMsgDataUpdate nodecfg;
    } update;
};

#define CLUSTER_PROTO_VER 0 /* Cluster bus protocol version. 群集总线协议版本*/

typedef struct {
    char sig[4];        /* Siganture "RCmb" (Redis Cluster message bus). 签名"RCmb"（Redis集群消息总线）*/
    uint32_t totlen;    /* Total length of this message  消息的总长*/
    uint16_t ver;       /* Protocol version, currently set to 0. 协议版本*/
    uint16_t notused0;  /* 2 bytes not used. 未使用*/
    uint16_t type;      /* Message type 消息类型 */
    uint16_t count;     /* Only used for some kind of messages.  只用于某些消息*/
    uint64_t currentEpoch;  /* The epoch accordingly to the sending node. 发送节点的epoch*/
    uint64_t configEpoch;   /* The config epoch if it's a master, or the last
                               epoch advertised by its master if it is a
                               slave.  配置的epoch，如果是一个主节点，或最新的epoch如果是从节点。*/
    uint64_t offset;    /* Master replication offset if node is a master or
                           processed replication offset if node is a slave.
                                                                          如果节点是从属的，则主复制偏移量,如果节点是主处理复制的偏移量。 */
    char sender[CLUSTER_NAMELEN]; /* Name of the sender node  发送节点的名称*/
    unsigned char myslots[CLUSTER_SLOTS/8];
    char slaveof[CLUSTER_NAMELEN];
    char notused1[32];  /* 32 bytes reserved for future usage. */
    uint16_t port;      /* Sender TCP base port */
    uint16_t flags;     /* Sender node flags */
    unsigned char state; /* Cluster state from the POV of the sender */
    unsigned char mflags[3]; /* Message flags: CLUSTERMSG_FLAG[012]_... */
    union clusterMsgData data;
} clusterMsg;

#define CLUSTERMSG_MIN_LEN (sizeof(clusterMsg)-sizeof(union clusterMsgData))

/* Message flags better specify the packet content or are used to
 * provide some information about the node state.
 * 消息标志更好地指定包内容，或用于提供关于节点状态的一些信息。 */
#define CLUSTERMSG_FLAG0_PAUSED (1<<0) /* Master paused for manual failover.  主节点暂停手动故障转移*/
#define CLUSTERMSG_FLAG0_FORCEACK (1<<1) /* Give ACK to AUTH_REQUEST even if
                                            master is up. */

/* ---------------------- API exported outside cluster.c -------------------- */
clusterNode *getNodeByQuery(client *c, struct redisCommand *cmd, robj **argv, int argc, int *hashslot, int *ask);
int clusterRedirectBlockedClientIfNeeded(client *c);
void clusterRedirectClient(client *c, clusterNode *n, int hashslot, int error_code);

#endif /* __CLUSTER_H */
