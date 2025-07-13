#pragma once

typedef struct PathTrapezoid PathTrapezoid;

struct PathTrapezoid {
    uint32_t       trap_id;
    PathTrapezoid *top_left;
    PathTrapezoid *top_rigth;
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

typedef struct Portal {
    uint32_t id;
} Portal;
typedef array(Portal) ArrayPortal;

typedef struct PathPlane {
    uint32_t           plane_id;
    Vec2fArray         vectors;
    PathTrapezoidArray trapezoids;
    uint32_t           next_trap_idx;
    ArrayNode          sink_nodes;
    ArrayNode          xnodes;
    ArrayNode          ynodes;
    ArrayPortal        portals;
    uint32_t           h000C;
    uint32_t           h0034;
    Node              *root_node;
} PathPlane;
typedef array(PathPlane) ArrayPathPlane;

typedef struct PathStaticData {
    ArrayPathPlane planes;
} PathStaticData;

typedef struct PathNode PathNode;
struct PathNode {
    bool      closed;
    float     cost_to_node;
    GmPos     pos;
    PathTrapezoid *trap_id;
    PathNode *parent;
};
typedef array(PathNode) PathNodeArray;

typedef struct PathContext {
    PathStaticData static_data;
    PathNodeArray  nodes;
} PathContext;

typedef struct Waypoint {
    GmPos    pos;
    uint32_t trap_id;
} Waypoint;
typedef array(Waypoint) WaypointArray;

typedef struct PrioQElem {
    float    cost;
    uint32_t trap_id;
} PrioQElem;

PathTrapezoid* FindTrapezoid(PathContext *context, GmPos pos);
PathTrapezoid* SearchTrapezoid(PathContext *context, Vec2f pos);

void PathFinding(PathContext *context, GmPos srcPos, GmPos dstPos, WaypointArray *waypoints);
