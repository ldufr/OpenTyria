#pragma once

typedef struct PathTrapezoid PathTrapezoid;

struct PathTrapezoid {
    uint32_t       trap_id;
    PathTrapezoid *top_left;
    PathTrapezoid *top_right;
    PathTrapezoid *bottom_left;
    PathTrapezoid *bottom_right;
    uint16_t       portal_left;
    uint16_t       portal_right;
    float          yt;
    float          yb;
    float          xtl;
    float          xtr;
    float          xbl;
    float          xbr;
};
typedef array(PathTrapezoid) PathTrapezoidArray;
typedef array(PathTrapezoid*) PathTrapezoidPtrArray;

typedef enum NodeType {
    NodeType_XNode = 0,
    NodeType_YNode = 1,
    NodeType_Sink  = 2,
} NodeType;

typedef struct Node Node;

typedef struct XNode {
    Vec2f pos;
    Vec2f dir;
    Node *left;
    Node *right;
} XNode;
typedef array(XNode) ArrayXNode;

typedef struct YNode {
    Vec2f pos;
    Node *above;
    Node *bellow;
} YNode;
typedef array(YNode) ArrayYNode;

typedef struct SinkNode {
    PathTrapezoid* trap;
} SinkNode;
typedef array(SinkNode) ArraySinkNode;

typedef struct Node {
    NodeType type;
    union
    {
        XNode xnode;
        YNode ynode;
        SinkNode sink_node;
    };
} Node;
typedef array(Node) ArrayNode;

typedef struct Portal Portal;
struct Portal {
    uint16_t portal_plane_id;
    uint16_t neighbor_plane_id;
    uint8_t  flags;
    Portal  *pair;
    uint32_t traps_count;
    PathTrapezoid **traps;
};
typedef array(Portal) ArrayPortal;

typedef struct PathPlane {
    uint32_t              plane_id;
    Vec2fArray            vectors;
    PathTrapezoidArray    trapezoids;
    uint32_t              next_trap_idx;
    ArrayNode             sink_nodes;
    ArrayNode             xnodes;
    ArrayNode             ynodes;
    ArrayPortal           portals;
    uint32_t              h000C;
    PathTrapezoidPtrArray portals_traps;
    Node                 *root_node;
} PathPlane;
typedef array(PathPlane) ArrayPathPlane;

typedef struct PathStaticData {
    ArrayPathPlane planes;
    uint32_t       traps_count;
} PathStaticData;

typedef struct PathFindPoint {
    GmPos pos;
    PathTrapezoid *trap;
} PathFindPoint;

typedef struct PathFindNode PathFindNode;
struct PathFindNode {
    bool          closed;
    float         cost_to_node;
    PathFindPoint point;
    PathFindNode *next;
};
typedef array(PathFindNode) PathFindNodeArray;

typedef struct PathBuildStep {
    Vec2f pos;
    Vec2f dir;
    uint16_t plane;
    PathTrapezoid *next_trap;
} PathBuildStep;
typedef array(PathBuildStep) PathBuildStepArray;

typedef struct PathContext {
    PathStaticData     static_data;
    PathFindNodeArray  nodes;
    PathHeapArray      prioq;
    PathBuildStepArray steps;
} PathContext;

typedef struct Waypoint {
    GmPos    pos;
    uint32_t trap_id;
} Waypoint;
typedef array(Waypoint) WaypointArray;

PathTrapezoid* FindTrapezoid(PathContext *context, GmPos pos);
PathTrapezoid* SearchTrapezoid(PathContext *context, Vec2f pos);

bool PathFinding(PathContext *context, GmPos src_pos, GmPos dst_pos, WaypointArray *waypoints);
