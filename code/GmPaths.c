#pragma once

PathTrapezoid* SearchTrapezoid(PathContext *context, Vec2f pos)
{
    PathTrapezoid* result = NULL;
    ArrayPathPlane *planes = &context->static_data.planes;
    GmPos pos2;
    pos2.x = pos.x;
    pos2.y = pos.y;
    for (size_t idx = planes->len - 1; idx < planes->len; --idx) {
        pos2.plane = idx;
        if ((result = FindTrapezoid(context, pos2)) != NULL)
            break;
    }

    printf("plane is %u\n", pos2.plane);
    return result;
}

PathTrapezoid* FindTrapezoid(PathContext *context, GmPos pos)
{
    ArrayPathPlane *planes = &context->static_data.planes;
    if (planes->len <= pos.plane)
        return NULL;

    PathPlane *plane = &planes->ptr[pos.plane];
    Node *node = plane->root_node;
    Vec2f fpos = { pos.x, pos.y };

    while (node) {
        switch (node->type) {
        case NodeType_XNode: {
            XNode *xnode = &node->xnode;
            Vec2f tmp = Vec2fSub(fpos, xnode->pos);
            float res = Vec2fCross(xnode->dir, tmp);
            if (res < 0) {
                node = xnode->left;
            } else {
                node = xnode->right;
            }
            break;
        }
        case NodeType_YNode: {
            YNode *ynode = &node->ynode;
            if (pos.y < ynode->pos.y) {
                node = ynode->bellow;
            } else if (pos.y > ynode->pos.y) {
                node = ynode->above;
            } else if (pos.x < ynode->pos.x) {
                node = ynode->bellow;
            } else {
                node = ynode->above;
            }
            break;
        }
        case NodeType_Sink:
            return node->sink_node.trap;
        }
    }

    return NULL;
}

void PathAddNode(
    PathContext *context,
    PathNode *parent,
    PathTrapezoid *trap,
    GmPos pos,
    float current_cost,
    float estimated_cost)
{
    assert(trap->trap_id < context->nodes.len);

    PathNode *pathNode = &context->nodes.ptr[trap->trap_id];
    pathNode->closed = false;
    pathNode->cost_to_node = current_cost;
    pathNode->pos = pos;
    pathNode->trap = trap;
    pathNode->parent = parent;

    PrioQElem elem;
    elem.cost = estimated_cost;
    elem.trap_id = trap->trap_id;

    // We may push the node multiple times to the priority queue, but only if the cost is smaller.
    // This means that we won't even pop the higher cost, which means we trade-off the size of the
    // prioq with the bookkeeping whenever the node move.
    PathPrioPush(&context->prioq, elem);
}

bool IsPointInTrap(GmPos *pos, PathTrapezoid *trap)
{
    // Check if the point is bellow or above the trapezoid.
    if (pos->y <= trap->yb || trap->yt <= pos->y)
        return false;

    // Return false if the point is to the left of the left side.
    Vec2f pt = (Vec2f) { trap->xtl, trap->yt };
    Vec2f pb = (Vec2f) { trap->xbl, trap->yb };
    Vec2f point = Vec2fSub(pt, pb);
    if (Vec2fCross(pos, point) < 0)
        return false;

    // Return false if the point is to the right of the right side.
    pt = (Vec2f) { trap->xtr, trap->yt };
    pb = (Vec2f) { trap->xbr, trap->yb };
    point = Vec2fSub(pt, pb);
    if (0 < Vec2fCross(pos, point))
        return false;

    return true;
}

void GetClosestPointOnLineSegment(GmPos pos, Vec2f p1, Vec2f p2, float *closest_point_dist, Vec2f *closest_point)
{
    Vec2f p = Vec2fSub(p2, p1);
    Vec2f v = Vec2fSub(pos, p1);

    if (p.x == 0.f && p.y == 0.f) {
        // Don't think that would ever make sense, so we add an assert to check if we actually need that.
        assert(p.x != 0.f || p.y != 0.f);

        float norm = Vec2fDot(v, v);
        if (norm < closest_point_dist) {
            *closest_point_dist = norm;
            *closest_point = p1;
        }
        return;
    }

    // We do the following in two steps.
    // 1. Calculate the coefficient of proj_b(a), that is the amount by which we multiply
    //    b to find the closest point to a.
    // 2. If this coefficient is less than 0 or greater than 1, it means that the closest
    //    intersection point is not in the line segment, since b represent the actual full line.
    // 3. If the intersection point is outside the line segment, we "clamp" it to p1 or p2.
    // 4. Calculate the vector from a to the closest point on the line segment.
    // 5. Calculate the norm from this new vector.

    float coeff = Vec2fDot(v, p) / Vec2fDot(p, p);
    Vec2f proj;

    if (0.f <= coeff && coeff <= 1.f) {
        proj.x = coeff * p.x + p1.x;
        proj.y = coeff * p.y + p1.y;
    } else if (1.f <= coeff) {
        proj.x = p2.x;
        proj.y = p2.y;
    } else if (coeff <= 0.f) {
        proj.x = p1.x;
        proj.y = p1.y;
    }

    Vec2f oproj = Vec2fSub(pos, proj);
    float norm = Vec2fDot(oproj, oproj)

    if (norm < closest_point_dist) {
        *closest_point_dist = norm;
        *closest_point = oproj;
    }
}

float DistToTrap(GmPos pos, PathTrapezoid *trap)
{
    if (IsPointInTrap(pos, trap))
        return 0.f;

    float dist = INFINITE;
    Vec2f point;

    Vec2f tl = (Vec2f) { trap->xtl, trap->yt };
    Vec2f tr = (Vec2f) { trap->xtr, trap->yt };
    Vec2f bl = (Vec2f) { trap->xbl, trap->yb };
    Vec2f br = (Vec2f) { trap->xbr, trap->yb };
    GetClosestPointOnLineSegment(pos, tl, t2, &dist, &point);
    GetClosestPointOnLineSegment(pos, tr, br, &dist, &point);
    GetClosestPointOnLineSegment(pos, br, bl, &dist, &point);
    GetClosestPointOnLineSegment(pos, bl, tl, &dist, &point);

    return dist;
}

void FindPointOnNextTrap(float min_left_x, float max_right_x, float next_y, Vec2f src_pos, Vec2f dst_pos, Vec2f *result)
{
    if (next_y == dst_pos.y)
    {
        float x1 = fclamp(src_pos.x, min_left_x, max_right_x);
        float x2 = fclamp(dst_pos.x, min_left_x, max_right_x);
        result->x = x1 * .1f + x2 * .9f;
        result->y = next_y;
    }

    // Line between dst_pos and srcPos is y = ax+b, but assume srcPos is the origin, we get
    // (dst_pos.y - srcPos.y) = a*(dst_pos.x - srcPos.x) + b, where b = 0
    // => a = (dst_pos.y - srcPos.y) / (dst_pos.x - srcPos.x)
    // Find where the line intersect the re-centered y. (i.e. y - srcPos.y)
    // (y - srcPos.y) = a*x => (y - srcPos.y) / a = x
    // => (y - srcPos.y) * 1/a = x
    // => (y - srcPos.y) * ((dst_pos.x - srcPos.x) / (dst_pos.y - srcPos.y))

    // Calculate the "a" from the src_pos to dst_pos
    float a_recip = (dst_pos.x - src_pos.x) / (dst_pos.y - src_pos.y);

    // Calculate the x where that would intersect the horizontal line at "next_y".
    result->x = ((next_y - src_pos.y) * a_recip) + src_pos.x;
    result->y = next_y;
}

void PathVisitTrap(PathContext *context, PathNode *curr_node, Vec2f dst_pos, PathTrapezoid *neighbour, Vec2f cross_pos, float max_cost)
{
    float dist_to_cross_point = Vec2fDist(curr_node->pos, cross_pos);
    float dist_to_end = Vec2fDist(cross_pos, dst_pos);

    float cost_to_neighbour = curr_node->cost + dist_to_cross_point;
    float new_estimated_cost = cost_to_neighbour + dist_to_end;

    // That doesn't seem necessary...
    GmPos cross_pos2;
    cross_pos2.x = cross_pos.x;
    cross_pos2.y = cross_pos.y;
    cross_pos2.plane = curr_node->pos.plane;

    PathAddNode(context, curr_node, neighbour, cross_pos2, cost_to_neighbour, new_estimated_cost);
}

void PathVisitAbove(PathContext *context, PathNode *curr_node, Vec2f dst_pos, PathTrapezoid *neighbour, float max_cost)
{
    Vec2f cross_pos;
    PathTrapezoid *trap = curr_node->trap;
    float xl = fmax(neighbour->xbl, trap->xtl);
    float xr = fmin(neighbour->xbr, trap->xtr);
    FindPointOnNextTrap(xl, xr, neighbour->yb, curr_node->pos, dst_pos, neighbour, &cross_pos);
    PathVisitTrap(context, curr_node, dst_pos, neighbour, cross_pos, max_cost);
}

void PathVisitBellow(PathContext *context, PathNode *curr_node, Vec2f dst_pos, PathTrapezoid *neighbour, float max_cost)
{
    Vec2f cross_pos;
    PathTrapezoid *trap = curr_node->trap;
    float xl = fmax(neighbour->xtl, trap->xbl);
    float xr = fmin(neighbour->xtr, trap->xbr);
    FindPointOnNextTrap(xl, xr, neighbour->yt, curr_node->pos, dst_pos, neighbour, cross_pos);
    PathVisitTrap(context, curr_node, dst_pos, neighbour, &cross_pos, max_cost);
}

bool PathFinding(PathContext *context, GmPos src_pos, GmPos dst_pos, WaypointArray *waypoints)
{
    const float MAX_COST = 3000.f;

    PathTrapezoid *src_trap, *dst_trap;

    if ((src_trap = FindTrapezoid(context, src_pos)) == NULL)
        return false;
    if ((dst_trap = FindTrapezoid(context, dst_pos)) == NULL)
        return false;

    if (src_trap == dst_trap) {
        Waypoint *waypoint = array_push(waypoints, 1);
        waypoint->pos = dst_pos;
        waypoint->trap_id = src_trap->trap_id;
        return true;
    }

    PathAddNode(context, NULL, src_trap, src_pos, 0, INFINITE);
    while (context->prioq.len != 0) {
        PrioQElem top = PathPrioPop(&context->prioq);
        assert(top.trap_id < context->nodes.len);
        PathNode *curr_node = &context->nodes.ptr[top.trap_id];
        curr_node->closed = true;

        PathTrapezoid *curr_trap = curr_node->trap;
        if (curr_node->trap == dst_trap) {
            // finish building
            break;
        }

        // Not sure yet why we keep track of that...
        // float cur_dist = DistToTrap(dst_pos, curr_node->trap);
        // if (cur_dist < best_dist) {
        //     best_dist = cur_dist;
        //     dstSegment.trap = curr_node->trap;
        // }

        if (curr_trap->top_left) {
            PathVisitAbove(context, curr_node, dst_pos, curr_trap->top_left, MAX_COST);
        }

        if (curr_trap->top_right) {
            PathVisitAbove(context, curr_node, dst_pos, curr_trap->top_right, MAX_COST);
        }

        if (curr_trap->bottom_left) {
            PathVisitBellow(context, curr_node, dst_pos, curr_trap->bottom_left, MAX_COST);
        }

        if (curr_trap->bottom_right) {
            PathVisitBellow(context, curr_node, dst_pos, curr_trap->bottom_left, MAX_COST);
        }

        if (curr_trap->portal_left != UINT16_MAX) {
            // visit portal
        }

        if (curr_trap->portal_right != UINT16_MAX) {
            // visit portal
        }
    }
}
