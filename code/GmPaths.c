#pragma once

PathTrapezoid* FindTrapezoid(PathContext *context, GmPos pos)
{
    ArrayPathPlane *planes = &context->static_data.planes;
    if (planes->len <= pos.plane)
        return NULL;

    PathPlane *plane = &array_at(planes, pos.plane);
    Node *node = plane->root_node;

    while (node) {
        switch (node->type) {
        case NodeType_XNode: {
            XNode *xnode = &node->xnode;
            Vec2f tmp = Vec2fSub(pos.v2, xnode->pos);
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

float PathReciproc(float value)
{
    static_assert(sizeof(float) == sizeof(uint32_t), "sizeof(float) == sizeof(uint32_t)");
    static uint32_t LookupTable[256] = {
        0x007F8040, 0x007E823D, 0x007D8631, 0x007C8C16, 0x007B93E6, 0x007A9D9D, 0x0079A934, 0x0078B6A6,
        0x0077C5EE, 0x0076D705, 0x0075E9E8, 0x0074FE91, 0x007414FA, 0x00732D1F, 0x007246FB, 0x00716289,
        0x00707FC4, 0x006F9EA8, 0x006EBF2F, 0x006DE156, 0x006D0518, 0x006C2A70, 0x006B515A, 0x006A79D1,
        0x0069A3D2, 0x0068CF59, 0x0067FC60, 0x00672AE4, 0x00665AE2, 0x00658C54, 0x0064BF38, 0x0063F389,
        0x00632943, 0x00626063, 0x006198E5, 0x0060D2C6, 0x00600E01, 0x005F4A93, 0x005E887A, 0x005DC7B0,
        0x005D0834, 0x005C4A01, 0x005B8D14, 0x005AD16A, 0x005A1700, 0x00595DD3, 0x0058A5DF, 0x0057EF21,
        0x00573997, 0x0056853D, 0x0055D210, 0x0055200D, 0x00546F32, 0x0053BF7C, 0x005310E7, 0x00526371,
        0x0051B717, 0x00510BD7, 0x005061AE, 0x004FB899, 0x004F1095, 0x004E69A0, 0x004DC3B8, 0x004D1ED9,
        0x004C7B02, 0x004BD830, 0x004B3660, 0x004A9590, 0x0049F5BF, 0x004956E8, 0x0048B90B, 0x00481C24,
        0x00478032, 0x0046E532, 0x00464B22, 0x0045B201, 0x004519CB, 0x0044827F, 0x0043EC1A, 0x0043569B,
        0x0042C1FF, 0x00422E45, 0x00419B6A, 0x0041096D, 0x0040784B, 0x003FE803, 0x003F5892, 0x003EC9F8,
        0x003E3C31, 0x003DAF3C, 0x003D2318, 0x003C97C2, 0x003C0D39, 0x003B837B, 0x003AFA86, 0x003A7258,
        0x0039EAF0, 0x0039644D, 0x0038DE6C, 0x0038594B, 0x0037D4EA, 0x00375147, 0x0036CE5F, 0x00364C32,
        0x0035CABE, 0x00354A01, 0x0034C9FA, 0x00344AA7, 0x0033CC07, 0x00334E19, 0x0032D0DA, 0x0032544A,
        0x0031D867, 0x00315D2F, 0x0030E2A2, 0x003068BE, 0x002FEF82, 0x002F76EB, 0x002EFEFA, 0x002E87AB,
        0x002E1100, 0x002D9AF5, 0x002D258A, 0x002CB0BD, 0x002C3C8D, 0x002BC8FA, 0x002B5601, 0x002AE3A1,
        0x002A71DA, 0x002A00AA, 0x00299010, 0x0029200B, 0x0028B099, 0x002841BA, 0x0027D36C, 0x002765AE,
        0x0026F880, 0x00268BDF, 0x00261FCC, 0x0025B445, 0x00254948, 0x0024DED5, 0x002474EB, 0x00240B89,
        0x0023A2AD, 0x00233A57, 0x0022D286, 0x00226B39, 0x0022046E, 0x00219E25, 0x0021385D, 0x0020D315,
        0x00206E4C, 0x00200A01, 0x001FA633, 0x001F42E1, 0x001EE00A, 0x001E7DAE, 0x001E1BCB, 0x001DBA61,
        0x001D596E, 0x001CF8F3, 0x001C98ED, 0x001C395D, 0x001BDA41, 0x001B7B99, 0x001B1D63, 0x001ABF9F,
        0x001A624D, 0x001A056A, 0x0019A8F7, 0x00194CF3, 0x0018F15D, 0x00189634, 0x00183B77, 0x0017E126,
        0x00178740, 0x00172DC4, 0x0016D4B2, 0x00167C08, 0x001623C7, 0x0015CBEC, 0x00157478, 0x00151D6A,
        0x0014C6C2, 0x0014707D, 0x00141A9D, 0x0013C51F, 0x00137005, 0x00131B4C, 0x0012C6F4, 0x001272FC,
        0x00121F65, 0x0011CC2C, 0x00117953, 0x001126D7, 0x0010D4B8, 0x001082F7, 0x00103191, 0x000FE087,
        0x000F8FD8, 0x000F3F83, 0x000EEF87, 0x000E9FE5, 0x000E509C, 0x000E01AA, 0x000DB310, 0x000D64CC,
        0x000D16DF, 0x000CC948, 0x000C7C05, 0x000C2F18, 0x000BE27E, 0x000B9638, 0x000B4A45, 0x000AFEA5,
        0x000AB356, 0x000A6859, 0x000A1DAC, 0x0009D350, 0x00098944, 0x00093F88, 0x0008F61A, 0x0008ACFB,
        0x0008642A, 0x00081BA6, 0x0007D36F, 0x00078B84, 0x000743E6, 0x0006FC93, 0x0006B58B, 0x00066ECD,
        0x0006285A, 0x0005E231, 0x00059C50, 0x000556B9, 0x0005116A, 0x0004CC63, 0x000487A3, 0x0004432A,
        0x0003FEF8, 0x0003BB0C, 0x00037766, 0x00033405, 0x0002F0E9, 0x0002AE12, 0x00026B7F, 0x0002292F,
        0x0001E723, 0x0001A559, 0x000163D3, 0x0001228E, 0x0000E18B, 0x0000A0C9, 0x00006048, 0x00002008,
    };

    uint32_t u32val;
    memcpy(&u32val, &value, sizeof(u32val));

    uint32_t tmp1 = (u32val & 0xFF800000) + 0x817FFFFF;
    uint32_t tmp2 = LookupTable[(u32val & 0x7FFFFF) >> 15];
    uint32_t tmp3 = tmp2 - tmp1;

    memcpy(&value, &tmp3, sizeof(value));
    return value;
}

float PathSqrt(float value)
{
    static_assert(sizeof(float) == sizeof(uint32_t), "sizeof(float) == sizeof(uint32_t)");
    static uint32_t LookupTable[256] = {
        0x0035B99D, 0x00366D95, 0x003720DC, 0x0037D374, 0x00388560, 0x003936A0, 0x0039E737, 0x003A9728,
        0x003B4673, 0x003BF51A, 0x003CA320, 0x003D5086, 0x003DFD4D, 0x003EA978, 0x003F5509, 0x003FFFFF,
        0x0040AA5E, 0x00415427, 0x0041FD5C, 0x0042A5FD, 0x00434E0D, 0x0043F58C, 0x00449C7D, 0x004542E1,
        0x0045E8B8, 0x00468E05, 0x004732C9, 0x0047D705, 0x00487ABB, 0x00491DEB, 0x0049C098, 0x004A62C1,
        0x004B0469, 0x004BA591, 0x004C463A, 0x004CE664, 0x004D8612, 0x004E2544, 0x004EC3FC, 0x004F623A,
        0x004FFFFF, 0x00509D4E, 0x00513A26, 0x0051D689, 0x00527277, 0x00530DF3, 0x0053A8FC, 0x00544394,
        0x0054DDBC, 0x00557774, 0x005610BE, 0x0056A99B, 0x0057420B, 0x0057DA0F, 0x005871A9, 0x005908D8,
        0x00599F9F, 0x005A35FD, 0x005ACBF5, 0x005B6185, 0x005BF6B0, 0x005C8B76, 0x005D1FD8, 0x005DB3D7,
        0x005E4773, 0x005EDAAD, 0x005F6D86, 0x005FFFFF, 0x00609219, 0x006123D4, 0x0061B530, 0x0062462F,
        0x0062D6D2, 0x00636719, 0x0063F704, 0x00648694, 0x006515CB, 0x0065A4A8, 0x0066332D, 0x0066C15A,
        0x00674F2F, 0x0067DCAD, 0x006869D6, 0x0068F6A8, 0x00698326, 0x006A0F50, 0x006A9B26, 0x006B26A8,
        0x006BB1D8, 0x006C3CB7, 0x006CC743, 0x006D517F, 0x006DDB6A, 0x006E6506, 0x006EEE52, 0x006F7750,
        0x006FFFFF, 0x00708861, 0x00711076, 0x0071983E, 0x00721FBA, 0x0072A6EA, 0x00732DCF, 0x0073B469,
        0x00743AB9, 0x0074C0C0, 0x0075467D, 0x0075CBF2, 0x0076511E, 0x0076D602, 0x00775A9F, 0x0077DEF5,
        0x00786304, 0x0078E6CE, 0x00796A52, 0x0079ED90, 0x007A708A, 0x007AF33F, 0x007B75B1, 0x007BF7DF,
        0x007C79CA, 0x007CFB72, 0x007D7CD8, 0x007DFDFB, 0x007E7EDE, 0x007EFF7F, 0x007F7FDF, 0x007FFFFF,
        0x00807FC0, 0x0080FF01, 0x00817DC6, 0x0081FC0F, 0x008279DE, 0x0082F734, 0x00837412, 0x0083F07B,
        0x00846C6E, 0x0084E7EE, 0x008562FB, 0x0085DD98, 0x008657C4, 0x0086D182, 0x00874AD2, 0x0087C3B6,
        0x00883C2E, 0x0088B43D, 0x00892BE2, 0x0089A31F, 0x008A19F6, 0x008A9066, 0x008B0672, 0x008B7C19,
        0x008BF15E, 0x008C6641, 0x008CDAC2, 0x008D4EE4, 0x008DC2A7, 0x008E360B, 0x008EA912, 0x008F1BBC,
        0x008F8E0B, 0x00900000, 0x0090719A, 0x0090E2DB, 0x009153C4, 0x0091C456, 0x00923490, 0x0092A475,
        0x00931405, 0x00938341, 0x0093F228, 0x009460BD, 0x0094CF00, 0x00953CF1, 0x0095AA92, 0x009617E2,
        0x009684E3, 0x0096F196, 0x00975DFA, 0x0097CA11, 0x009835DB, 0x0098A159, 0x00990C8C, 0x00997773,
        0x0099E211, 0x009A4C64, 0x009AB66F, 0x009B2031, 0x009B89AB, 0x009BF2DE, 0x009C5BCA, 0x009CC470,
        0x009D2CD0, 0x009D94EC, 0x009DFCC2, 0x009E6454, 0x009ECBA3, 0x009F32AF, 0x009F9978, 0x00A00000,
        0x00A06645, 0x00A0CC4A, 0x00A1320E, 0x00A19792, 0x00A1FCD6, 0x00A261DC, 0x00A2C6A2, 0x00A32B2B,
        0x00A38F75, 0x00A3F382, 0x00A45753, 0x00A4BAE6, 0x00A51E3E, 0x00A5815A, 0x00A5E43B, 0x00A646E1,
        0x00A6A94D, 0x00A70B7E, 0x00A76D77, 0x00A7CF35, 0x00A830BC, 0x00A89209, 0x00A8F31F, 0x00A953FD,
        0x00A9B4A4, 0x00AA1513, 0x00AA754D, 0x00AAD550, 0x00AB351D, 0x00AB94B4, 0x00ABF417, 0x00AC5345,
        0x00ACB23E, 0x00AD1103, 0x00AD6F95, 0x00ADCDF3, 0x00AE2C1D, 0x00AE8A15, 0x00AEE7DB, 0x00AF456E,
        0x00AFA2D0, 0x00B00000, 0x00B05CFE, 0x00B0B9CC, 0x00B11669, 0x00B172D6, 0x00B1CF13, 0x00B22B20,
        0x00B286FD, 0x00B2E2AC, 0x00B33E2B, 0x00B3997C, 0x00B3F49F, 0x00B44F93, 0x00B4AA5A, 0x00B504F3,
    };

    uint32_t u32val;
    memcpy(&u32val, &value, sizeof(u32val));

    uint32_t tmp1 = ((u32val >> 24) + 0x3F) << 23;
    uint32_t tmp2 = LookupTable[(u32val >> 16) & 0xFF];
    uint32_t tmp3 = tmp1 + tmp2;

    memcpy(&value, &tmp3, sizeof(value));
    return value;
}

float PathVec2fDist(Vec2f v1, Vec2f v2)
{
    float dx = v1.x - v2.x;
    float dy = v1.y - v2.y;
    return PathSqrt(dx * dx + dy * dy);
}

Vec2f PathVec2fUnit(Vec2f v)
{
    float tmp = Vec2fDot(v, v);
    tmp = PathSqrt(tmp);
    return Vec2fDiv(v, tmp);
}

void PathAddNode(
    PathContext *context,
    PathFindNode *parent,
    PathTrapezoid *trap,
    GmPos pos,
    float current_cost,
    float estimated_cost)
{
    PathFindNode *pathNode = &array_at(&context->nodes, trap->trap_id);
    pathNode->closed = false;
    pathNode->cost_to_node = current_cost;
    pathNode->point.pos = pos;
    pathNode->point.trap = trap;
    pathNode->next = parent;

    PathHeapNode elem;
    elem.cost = estimated_cost;
    elem.trap_id = trap->trap_id;

    #if 0
    printf(
        "astart_add_node: (%.4f, %.4f, %u), trap: %u, cost: %.4f, estimated_cost: %.4f\n",
        pos.x, pos.y, pos.plane,
        trap->trap_id,
        current_cost,
        estimated_cost
    );
    #endif

    // We may push the node multiple times to the priority queue, but only if the cost is smaller.
    // This means that we won't even pop the higher cost, which means we trade-off the size of the
    // prioq with the bookkeeping whenever the node move.
    PathHeapPush(&context->prioq, elem);
}

bool IsPointInTrap(GmPos pos, PathTrapezoid *trap)
{
    // Check if the point is bellow or above the trapezoid.
    if (pos.y <= trap->yb || trap->yt <= pos.y)
        return false;

    // Return false if the point is to the left of the left side.
    Vec2f pt = (Vec2f) { trap->xtl, trap->yt };
    Vec2f pb = (Vec2f) { trap->xbl, trap->yb };
    Vec2f point = Vec2fSub(pt, pb);
    if (Vec2fCross(pos.v2, point) < 0)
        return false;

    // Return false if the point is to the right of the right side.
    pt = (Vec2f) { trap->xtr, trap->yt };
    pb = (Vec2f) { trap->xbr, trap->yb };
    point = Vec2fSub(pt, pb);
    if (0 < Vec2fCross(pos.v2, point))
        return false;

    return true;
}

void GetClosestPointOnLineSegment(GmPos pos, Vec2f p1, Vec2f p2, float *closest_point_dist, Vec2f *closest_point)
{
    Vec2f p = Vec2fSub(p2, p1);
    Vec2f v = Vec2fSub(pos.v2, p1);

    if (p.x == 0.f && p.y == 0.f) {
        // Don't think that would ever make sense, so we add an assert to check if we actually need that.
        assert(p.x != 0.f || p.y != 0.f);

        float norm = Vec2fDot(v, v);
        if (norm < *closest_point_dist) {
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
    } else {
        assert(coeff <= 0.f);
        proj.x = p1.x;
        proj.y = p1.y;
    }

    Vec2f oproj = Vec2fSub(pos.v2, proj);
    float norm = Vec2fDot(oproj, oproj);

    if (norm < *closest_point_dist) {
        *closest_point_dist = norm;
        *closest_point = oproj;
    }
}

float DistToTrap(GmPos pos, PathTrapezoid *trap)
{
    if (IsPointInTrap(pos, trap))
        return 0.f;

    float dist = INFINITY;
    Vec2f point;

    Vec2f tl = (Vec2f) { trap->xtl, trap->yt };
    Vec2f tr = (Vec2f) { trap->xtr, trap->yt };
    Vec2f bl = (Vec2f) { trap->xbl, trap->yb };
    Vec2f br = (Vec2f) { trap->xbr, trap->yb };
    GetClosestPointOnLineSegment(pos, tl, tr, &dist, &point);
    GetClosestPointOnLineSegment(pos, tr, br, &dist, &point);
    GetClosestPointOnLineSegment(pos, br, bl, &dist, &point);
    GetClosestPointOnLineSegment(pos, bl, tl, &dist, &point);

    return dist;
}

void FindPointOnNextTrap(float min_left_x, float max_right_x, float next_y, Vec2f src_pos, Vec2f dst_pos, Vec2f *result)
{
    float percent = -1.f;

    if (src_pos.y != dst_pos.y)
    {
        float divisor = PathReciproc(dst_pos.y - src_pos.y);
        percent = (next_y - src_pos.y) * divisor;
    }

    if (percent < 0.f)
    {
        float src_x_clamp = fclampf(src_pos.x, min_left_x, max_right_x);
        float dst_x_clamp = fclampf(dst_pos.x, min_left_x, max_right_x);
        result->x = src_x_clamp * 0.8999999761581421f + dst_x_clamp * 0.1000000238418579f;
        result->y = next_y;
    }
    else
    {
        // Calculate the x where that would intersect the horizontal line at "next_y".
        float new_x = ((dst_pos.x - src_pos.x) * percent) + src_pos.x;
        result->x = fclampf(new_x, min_left_x, max_right_x);
        result->y = next_y;
    }

    #if 0
    printf(
        "find_point_on_next_trap: lx: %.4f, rx: %.4f, next_y: %.4f, current pos: (%.4f, %.4f), dst pos: (%.4f, %.4f), result: (%.4f, %.4f)\n",
        min_left_x,
        max_right_x,
        next_y,
        src_pos.x, src_pos.y,
        dst_pos.x, dst_pos.y,
        result->x, result->y
    );
    #endif
}

void PathVisitTrap(PathContext *context, PathFindNode *curr_node, Vec2f dst_pos, PathTrapezoid *neighbor, GmPos cross_pos, float max_cost)
{
    float dist_to_trap = PathVec2fDist(curr_node->point.pos.v2, cross_pos.v2);
    float cost_to_trap = curr_node->cost_to_node + dist_to_trap;

    if (max_cost <= cost_to_trap) {
         return;
    }

    Vec2f bottom_point;
    bottom_point.x = fclampf(dst_pos.x, neighbor->xbl, neighbor->xbr);
    bottom_point.y = neighbor->yb;
    float estimated_dest_bottom = PathVec2fDist(bottom_point, dst_pos);

    Vec2f top_point;
    top_point.x = fclampf(dst_pos.x, neighbor->xtl, neighbor->xtr);
    top_point.y = neighbor->yt;
    float estimated_dest_top = PathVec2fDist(top_point, dst_pos);

    float best_estimate = fminf(estimated_dest_bottom, estimated_dest_top);

    if (neighbor->yb <= dst_pos.y && dst_pos.y <= neighbor->yt)
        best_estimate -= 0.009999999776482582;

    float new_estimated_cost = cost_to_trap + best_estimate;

    // That doesn't seem necessary...
    //GmPos cross_pos2;
    //cross_pos2.x = cross_pos.x;
    //cross_pos2.y = cross_pos.y;
    //cross_pos2.plane = curr_node->pos.plane;

    PathAddNode(context, curr_node, neighbor, cross_pos, cost_to_trap, new_estimated_cost);
}

void PathVisitAbove(PathContext *context, PathFindNode *curr_node, Vec2f dst_pos, PathTrapezoid *neighbor, float max_cost)
{
    PathTrapezoid *trap = curr_node->point.trap;
    float xl = fmaxf(neighbor->xbl, trap->xtl);
    float xr = fminf(neighbor->xbr, trap->xtr);
    GmPos cross_pos;
    cross_pos.plane = curr_node->point.pos.plane;
    FindPointOnNextTrap(xl, xr, neighbor->yb, curr_node->point.pos.v2, dst_pos, &cross_pos.v2);
    PathVisitTrap(context, curr_node, dst_pos, neighbor, cross_pos, max_cost);
}

void PathVisitBellow(PathContext *context, PathFindNode *curr_node, Vec2f dst_pos, PathTrapezoid *neighbor, float max_cost)
{
    PathTrapezoid *trap = curr_node->point.trap;
    float xl = fmaxf(neighbor->xtl, trap->xbl);
    float xr = fminf(neighbor->xtr, trap->xbr);
    GmPos cross_pos;
    cross_pos.plane = curr_node->point.pos.plane;
    FindPointOnNextTrap(xl, xr, neighbor->yt, curr_node->point.pos.v2, dst_pos, &cross_pos.v2);
    PathVisitTrap(context, curr_node, dst_pos, neighbor, cross_pos, max_cost);
}

// Find the intersection point of two lines if it exist. `t1` and `t2` are set
// such that: `start1 + t1 * dir1 = start2 + t2 * dir2`.
bool FindIntersectionPoint(Vec2f start1, Vec2f dir1, Vec2f start2, Vec2f dir2, float *t1, float *t2)
{
    float d = Vec2fCross(dir1, dir2);

    // The two lines are parallel, there is no intersection point.
    if (d == 0.f) {
        return false;
    }

    Vec2f v = Vec2fSub(start1, start2);
    *t1 = Vec2fCross(dir2, v) / d;
    *t2 = Vec2fCross(dir1, v) / d;
    return true;
}

void PickNextPoint(Vec2f point1, Vec2f point2, Vec2f cur_pos, Vec2f dst_pos, Vec2f *result)
{
    Vec2f cur_to_dst = Vec2fSub(dst_pos, cur_pos);
    Vec2f point1_to_point2 = Vec2fSub(point2, point1);
    float t1, t2;

    if (FindIntersectionPoint(cur_pos, cur_to_dst, point1, point1_to_point2, &t1, &t2) && 0 <= t2) {
        t2 = fminf(t2, 1.f);
        result->x = point1.x + (point1_to_point2.x * t2);
        result->y = point1.y + (point1_to_point2.y * t2);
    } else {
        float norm2 = Vec2fDot(point1_to_point2, point1_to_point2);

        // The game uses a "fast reciprocal" function with lookup table, that
        // also can divide by 0, returning 1.6981e+038.
        // float divisor = norm2 != 0.f ? 1 / norm2 : 1.6981e+038f;
        float divisor = PathReciproc(norm2);

        t1 = Vec2fDot(point1, point1_to_point2) * divisor;
        t2 = Vec2fDot(point2, point1_to_point2) * divisor;

        t1 = fclampf(t1, 0.f, 1.f);
        t2 = fclampf(t2, 0.f, 1.f);

        float t = t2 * 0.1f + t1 * 0.9f;
        result->x = point1.x + point1_to_point2.x * t;
        result->y = point1.y + point1_to_point2.y * t;
    }
}

void PathVisitPortalLeft(PathContext *context, PathFindNode *curr_node, Vec2f dst_pos, uint16_t portal_id, float max_cost)
{
    ArrayPortal *portals = &array_at(&context->static_data.planes, curr_node->point.pos.plane).portals;
    Portal *portal = &array_at(portals, portal_id);

    assert(portal->pair->portal_plane_id == portal->neighbor_plane_id);
    assert(portal->pair->neighbor_plane_id == portal->portal_plane_id);

    // printf("'visit_portal_left portal id: %u\n", portal_id);

    if ((portal->flags & 0x4) != 0) {
        return;
    }

    // @Cleanup: todo
    // if (IsPlaneBlocked(context, portal->neighbor_plane_id))
    //     return;

    PathTrapezoid **traps = portal->pair->traps;
    for (size_t idx = 0; idx < portal->pair->traps_count; ++idx) {
        PathTrapezoid *portal_trap = traps[idx];
        PathFindNode *node = &array_at(&context->nodes, portal_trap->trap_id);

        // We can't visit this trapezoid again.
        if (node && node->closed)
            continue;

        Vec2f point1; // top point
        if (portal_trap->yt <= curr_node->point.trap->yt) {
            point1.x = portal_trap->xtr;
            point1.y = portal_trap->yt;
        } else {
            point1.x = curr_node->point.trap->xtl;
            point1.y = curr_node->point.trap->yt;
        }

        Vec2f point2; // bottom point
        if (curr_node->point.trap->yb <= portal_trap->yb) {
            point2.x = portal_trap->xbr;
            point2.y = portal_trap->yb;
        } else {
            point2.x = curr_node->point.trap->xbl;
            point2.y = curr_node->point.trap->yb;
        }

        if (point2.y <= point1.y) {
            GmPos crossing_pos;
            crossing_pos.plane = portal->pair->portal_plane_id;
            PickNextPoint(point1, point2, curr_node->point.pos.v2, dst_pos, &crossing_pos.v2);
            PathVisitTrap(context, curr_node, dst_pos, portal_trap, crossing_pos, max_cost);
        } else {
            // assert(!"shouldn't be possible");
        }
    }
}

void PathVisitPortalRight(PathContext *context, PathFindNode *curr_node, Vec2f dst_pos, uint16_t portal_id, float max_cost)
{
    ArrayPortal *portals = &array_at(&context->static_data.planes, curr_node->point.pos.plane).portals;
    Portal *portal = &array_at(portals, portal_id);

    assert(portal->pair->portal_plane_id == portal->neighbor_plane_id);
    assert(portal->pair->neighbor_plane_id == portal->portal_plane_id);

    // printf("'visit_portal_right portal id: %u\n", portal_id);

    if ((portal->flags & 0x4) != 0) {
        return;
    }

    // @Cleanup: todo
    // if (IsPlaneBlocked(context, portal->next_plane_id))
    //     return;

    PathTrapezoid **traps = portal->pair->traps;
    for (size_t idx = 0; idx < portal->pair->traps_count; ++idx) {
        PathTrapezoid *portal_trap = traps[idx];
        PathFindNode *node = &array_at(&context->nodes, portal_trap->trap_id);

        // We can't visit this trapezoid again.
        if (node && node->closed)
            continue;

        Vec2f point1; // top point
        if (portal_trap->yt <= curr_node->point.trap->yt) {
            point1.x = portal_trap->xtl;
            point1.y = portal_trap->yt;
        } else {
            point1.x = curr_node->point.trap->xtr;
            point1.y = curr_node->point.trap->yt;
        }

        Vec2f point2; // bottom point
        if (curr_node->point.trap->yb <= portal_trap->yb) {
            point2.x = portal_trap->xbl;
            point2.y = portal_trap->yb;
        } else {
            point2.x = curr_node->point.trap->xbr;
            point2.y = curr_node->point.trap->yb;
        }

        if (point2.y <= point1.y) {
            GmPos crossing_pos;
            crossing_pos.plane = portal->pair->portal_plane_id;
            PickNextPoint(point1, point2, curr_node->point.pos.v2, dst_pos, &crossing_pos.v2);
            PathVisitTrap(context, curr_node, dst_pos, portal_trap, crossing_pos, max_cost);
        } else {
            // assert(!"shouldn't be possible");
        }
    }
}

PathFindNode *PathReversePathFindNode(PathFindNode *head)
{
    size_t count = 0;
    PathFindNode *prev = NULL;
    while (head != NULL) {
        PathFindNode *next = head->next;
        head->next = prev;
        prev = head;
        head = next;
        count++;
    }
    return prev;
}

typedef struct PathFindBound {
    PathFindPoint point;
    size_t        step_id;
    Vec2f         vec;
} PathFindBound;

typedef struct PathBuildHelper {
    PathContext *context;
    PathFindBound left_bound;
    PathFindBound right_bound;
    PathFindPoint curr_start_point;
    uint16_t current_plane;
    GmPos last_pos;
    PathFindPoint src_point;
    PathFindPoint dst_point;
    WaypointArray *waypoints;
} PathBuildHelper;

float PathVec2fPreciseCross(Vec2f v1, Vec2f v2)
{
    if (v1.x == 0.f && v1.y == 0.f)
        return 0.f;
    if (v2.x == 0.f && v2.y == 0.f)
        return 0.f;

    float cross_prod = Vec2fCross(v1, v2);
    if (-0.01f < cross_prod && cross_prod < 0.01f)
        return 0.f;

    if (cross_prod <= -1.f || 1.f <= cross_prod)
        return cross_prod;

    Vec2f v1_unit = PathVec2fUnit(v1);
    Vec2f v2_unit = PathVec2fUnit(v2);

    cross_prod = Vec2fCross(v1_unit, v2_unit);
    if (cross_prod <= -0.01f || 0.01f <= cross_prod)
        return cross_prod;
    
    return 0.f;
}

void PathBuildAddWaypointAndReduce(PathBuildHelper *helper, PathFindPoint new_point, Vec2f from_pos)
{
#if 0
    printf(
        "path_build_add_waypoint_and_reduce: new: %.4f, %.4f, %u, trap: %u, from: %.4f, %.4f\n",
        new_point.pos.x, new_point.pos.y, new_point.pos.plane,
        new_point.trap->trap_id,
        from_pos.x, from_pos.y
    );
#endif

    PathBuildStepArray *steps = &helper->context->steps;
    WaypointArray *waypoints = helper->waypoints;

    for (size_t idx = 0; idx < steps->len; ++idx) {
        PathBuildStep step = steps->ptr[idx];

        Vec2f best_pos;
        if (!Vec2fIsZero(step.dir)) {
            float t, s;

            Vec2f to_point = Vec2fSub(new_point.pos.v2, from_pos);
            FindIntersectionPoint(step.pos, step.dir, from_pos, to_point, &t, &s);

            if ( -0.01f <= t ) {
                best_pos = step.pos;
            } else if (1.01 < t) {
                best_pos = Vec2fAdd(step.pos, step.dir);
            } else {
                best_pos.x = step.pos.x + (t * step.dir.x);
                best_pos.y = step.pos.y + (t * step.dir.y);
            }
        } else {
            best_pos = step.pos;
        }

        if (!Vec2fEqual(helper->last_pos.v2, best_pos) || helper->last_pos.plane != step.plane) {
            helper->last_pos.v2 = best_pos;
            helper->last_pos.plane = step.plane;

            if (waypoints->len != 0) {
                Waypoint *prev_wp = &waypoints->ptr[waypoints->len - 1];
                if (Vec2fEqual(best_pos, prev_wp->pos.v2)) {
                    --waypoints->len;
                }
            }

            Waypoint *wp = array_push(helper->waypoints, 1);
            wp->pos.v2 = best_pos;
            wp->pos.plane = step.plane;
            wp->trap_id = step.next_trap->trap_id;
        }
    }

    array_clear(steps);
    if (!Vec2fEqual(helper->last_pos.v2, new_point.pos.v2)) {
        helper->last_pos = new_point.pos;

        if (waypoints->len != 0) {
            Waypoint *prev_wp = &waypoints->ptr[waypoints->len - 1];
            if (Vec2fEqual(new_point.pos.v2, prev_wp->pos.v2)) {
                --waypoints->len;
            }
        }

        Waypoint *wp = array_push(helper->waypoints, 1);
        wp->pos = new_point.pos;
        wp->trap_id = new_point.trap->trap_id;
    }
}

void PathBuildAddWaypoint(PathBuildHelper *helper, Vec2f left, Vec2f right, PathFindNode **node)
{
#if 0
    printf(
        "path_build_add_waypoint: left: %.4f, %.4f, right: %.4f, %.4f, current_trap_id: %u\n",
        left.x, left.y,
        right.x, right.y,
        (*node)->point.trap->trap_id
    );
#endif

    PathFindBound *bound_used;
    if (Vec2fEqual(helper->left_bound.point.pos.v2, left)) {
        bound_used = &helper->left_bound;
        helper->right_bound.point = helper->left_bound.point;
        helper->right_bound.step_id = helper->left_bound.step_id;
        *node = &array_at(&helper->context->nodes, helper->left_bound.point.trap->trap_id);
    } else if (Vec2fEqual(helper->right_bound.point.pos.v2, right)) {
        bound_used = &helper->right_bound;
        helper->left_bound.point = helper->right_bound.point;
        helper->left_bound.step_id = helper->right_bound.step_id;
        *node = &array_at(&helper->context->nodes, helper->right_bound.point.trap->trap_id);
    } else {
        float dx = left.x - helper->curr_start_point.pos.v2.x;
        float dy = left.y - helper->curr_start_point.pos.v2.y;
        float lval = dy * dy + dx * dx;

        dx = right.x - helper->curr_start_point.pos.v2.x;
        dy = right.y - helper->curr_start_point.pos.v2.y;
        float rval = dy * dy + dx * dx;

        if (lval < rval)
          bound_used = &helper->left_bound;
        else
          bound_used = &helper->right_bound;
    }

    array_resize(&helper->context->steps, bound_used->step_id);

    PathBuildAddWaypointAndReduce(helper, bound_used->point, helper->curr_start_point.pos.v2);
    helper->curr_start_point = bound_used->point;
    helper->right_bound.vec.x = 0.0;
    helper->right_bound.vec.y = 0.0;
    helper->left_bound.vec.y = 0.0;
    helper->left_bound.vec.x = 0.0;
    helper->current_plane = bound_used->point.pos.plane;
}

void PathBuildProcessNext(PathBuildHelper *helper, Vec2f left, Vec2f right, PathTrapezoid *next_trap, PathFindNode **node)
{
    Vec2f to_left = Vec2fSub(left, helper->curr_start_point.pos.v2);
    Vec2f to_right = Vec2fSub(right, helper->curr_start_point.pos.v2);

#if 0
    printf(
        "path_build_process_next: left: %.4f, %.4f, right: %.4f, %.4f, current_trap_id: %u, next_trap_id: %u\n",
        left.x, left.y,
        right.x, right.y,
        (*node)->point.trap->trap_id,
        next_trap->trap_id
    );
#endif

    PathFindBound new_left_bound;
    if (PathVec2fPreciseCross(to_left, helper->left_bound.vec) < 0.f) {
        // to_left is to the left of the left_bound, so we keep the left bound
        to_left = helper->left_bound.vec;
        new_left_bound = helper->left_bound;
    } else {
        new_left_bound.vec = to_left;
        new_left_bound.point.pos.v2 = left;
        new_left_bound.point.pos.plane = helper->current_plane;
        new_left_bound.point.trap = next_trap;
        new_left_bound.step_id = helper->context->steps.len;
    }

    PathFindBound new_right_bound;
    if (0.f < PathVec2fPreciseCross(to_right, helper->right_bound.vec)) {
        // to_right is to the right of the right_bound, so we keep the right bound
        to_right = helper->right_bound.vec;
        new_right_bound = helper->right_bound;
    } else {
        new_right_bound.vec = to_right;
        new_right_bound.point.pos.v2 = right;
        new_right_bound.point.pos.plane = helper->current_plane;
        new_right_bound.point.trap = next_trap;
        new_right_bound.step_id = helper->context->steps.len;
    }

    if (Vec2fIsZero(helper->left_bound.vec) || PathVec2fPreciseCross(to_left, to_right) <= 0.f) {
        helper->left_bound = new_left_bound;
        helper->right_bound = new_right_bound;
        *node = (*node)->next;
    } else {
        Vec2f ltmp = new_left_bound.point.pos.v2;
        Vec2f rtmp = new_right_bound.point.pos.v2;
        PathBuildAddWaypoint(helper, ltmp, rtmp, node);
    }
}

bool PathAddLast(PathBuildHelper *helper, PathFindNode **node, PathFindPoint new_point)
{
#if 0
    printf(
        "path_add_last: current_trap_id: %u, dst: %.4f, %.4f, %u\n",
        (*node)->point.trap->trap_id,
        new_point.pos.x, new_point.pos.y, new_point.pos.plane
    );
#endif

    Vec2f to_new_point = Vec2fSub(new_point.pos.v2, helper->curr_start_point.pos.v2);

    if (0 <= PathVec2fPreciseCross(to_new_point, helper->left_bound.vec)) {
        if (PathVec2fPreciseCross(to_new_point, helper->right_bound.vec) <= 0) {
            // [left, to_new_point, right]
            PathBuildAddWaypointAndReduce(helper, new_point, helper->curr_start_point.pos.v2);
            return true;
        } else {
            // [left, right, to_new_point]
            helper->right_bound.vec.x = 0.0;
            helper->right_bound.vec.y = 0.f;
            helper->left_bound.point = helper->right_bound.point;

            *node = &array_at(&helper->context->nodes, helper->right_bound.point.trap->trap_id);
            array_resize(&helper->context->steps, helper->right_bound.step_id);

            PathBuildAddWaypointAndReduce(helper, helper->right_bound.point, helper->curr_start_point.pos.v2);
            helper->curr_start_point = helper->right_bound.point;
            helper->current_plane = helper->right_bound.point.pos.plane;
            return false;
        }
    } else {
        // [to_new_point, left, right]
        helper->left_bound.vec.x = 0.0;
        helper->left_bound.vec.y = 0.f;
        helper->right_bound.point = helper->left_bound.point;

        *node = &array_at(&helper->context->nodes, helper->left_bound.point.trap->trap_id);
        array_resize(&helper->context->steps, helper->left_bound.step_id);

        PathBuildAddWaypointAndReduce(helper, helper->left_bound.point, helper->curr_start_point.pos.v2);
        helper->curr_start_point = helper->left_bound.point;
        helper->current_plane = helper->left_bound.point.pos.plane;
        return false;
    }
}

bool GetPortalAndCheckTrapAdjacent(PathBuildHelper *helper, uint32_t portal_id, PathTrapezoid *trap, struct Portal **result)
{
    if (portal_id == 0xFFFF)
        return false;

    PathPlane *plane = &array_at(&helper->context->static_data.planes, helper->current_plane);
    Portal *portal = &array_at(&plane->portals, portal_id);
    Portal *pair = portal->pair;

    for (uint32_t idx = 0; idx < pair->traps_count; ++idx) {
        if (pair->traps[idx] == trap) {
            *result = portal;
            return true;
        }
    }

    return false;
}

void PathCreateWaypoints(
    PathContext *context,
    PathFindPoint src_point, PathFindPoint dst_point,
    WaypointArray *waypoints)
{
    const int MAX_LOOP = 0xBB8;

    PathBuildHelper helper = {0};
    helper.context = context;
    helper.left_bound.point = src_point;
    helper.right_bound.point = src_point;
    helper.curr_start_point = src_point;
    helper.current_plane = src_point.pos.plane;
    helper.src_point = src_point;
    helper.dst_point = dst_point;
    helper.waypoints = waypoints;

    array_clear(&context->steps);
    PathFindNode *curr_node = &array_at(&context->nodes, src_point.trap->trap_id);

    for (int loop = 0; ; ++loop) {
        assert(loop < MAX_LOOP);

        Portal *portal;

        PathTrapezoid *curr_trap = curr_node->point.trap;
        PathFindNode *next_node = curr_node->next;

        if (!next_node || curr_trap == dst_point.trap) {
            if (PathAddLast(&helper, &curr_node, dst_point))
                break;
            continue;
        }

        PathTrapezoid *next_trap = curr_node->next->point.trap;
        if (next_trap == curr_trap->top_left || next_trap == curr_trap->top_right) {
            Vec2f left, right;
            left.x = fmaxf(curr_trap->xtl, next_trap->xbl);
            left.y = curr_trap->yt;
            right.x = fminf(curr_trap->xtr, next_trap->xbr);
            right.y = curr_trap->yt;
            PathBuildProcessNext(&helper, left, right, next_trap, &curr_node);
        } else if (next_trap == curr_trap->bottom_left || next_trap == curr_trap->bottom_right) {
            Vec2f left, right;
            left.x = fmaxf(curr_trap->xbl, next_trap->xtl);
            left.y = curr_trap->yb;
            right.x = fminf(curr_trap->xbr, next_trap->xtr);
            right.y = curr_trap->yb;
            PathBuildProcessNext(&helper, right, left, next_trap, &curr_node);
        } else if (GetPortalAndCheckTrapAdjacent(&helper, curr_trap->portal_right, next_trap, &portal)) {
            Vec2f top;
            if (next_trap->yt <= curr_trap->yt) {
                top.x = next_trap->xtl;
                top.y = next_trap->yt;
            } else {
                top.x = curr_trap->xtr;
                top.y = curr_trap->yt;
            }

            Vec2f bottom;
            if (curr_trap->yb <= next_trap->yb) {
                bottom.x = next_trap->xbl;
                bottom.y = next_trap->yb;
            } else {
                bottom.x = curr_trap->xbr;
                bottom.y = curr_trap->yb;
            }

            helper.current_plane = portal->neighbor_plane_id;

            PathBuildStep *step = array_push(&context->steps, 1);
            step->pos.x = top.x;
            step->pos.y = top.y;
            step->dir.x = bottom.x - top.x;
            step->dir.y = bottom.y - top.y;
            step->plane = helper.current_plane;
            step->next_trap = next_trap;
            PathBuildProcessNext(&helper, top, bottom, next_trap, &curr_node);
        }  else if (GetPortalAndCheckTrapAdjacent(&helper, curr_trap->portal_left, next_trap, &portal)) {
            Vec2f top;
            if (next_trap->yt <= curr_trap->yt) {
                top.x = next_trap->xtr;
                top.y = next_trap->yt;
            } else {
                top.x = curr_trap->xtl;
                top.y = curr_trap->yt;
            }

            Vec2f bottom;
            if (curr_trap->yb <= next_trap->yb) {
                bottom.x = next_trap->xbr;
                bottom.y = next_trap->yb;
            } else {
                bottom.x = curr_trap->xbl;
                bottom.y = curr_trap->yb;
            }

            helper.current_plane = portal->neighbor_plane_id;

            PathBuildStep *step = array_push(&context->steps, 1);
            step->pos.x = top.x;
            step->pos.y = top.y;
            step->dir.x = bottom.x - top.x;
            step->dir.y = bottom.y - top.y;
            step->plane = helper.current_plane;
            step->next_trap = next_trap;
            PathBuildProcessNext(&helper, bottom, top, next_trap, &curr_node);
        } else {
            assert(!"Shouldn't be possible");
            break;
        }
    }
}

bool PathFinding(PathContext *context, GmPos src_pos, GmPos dst_pos, WaypointArray *waypoints)
{
    const float MAX_COST = INFINITY;

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

    array_clear(&context->prioq);
    array_resize(&context->nodes, context->static_data.traps_count);
    memset(context->nodes.ptr, 0, context->nodes.len * sizeof(*context->nodes.ptr));

    PathAddNode(context, NULL, src_trap, src_pos, 0, INFINITY);

    PathHeapNode top;
    while (PathHeapPop(&context->prioq, &top)) {
        assert(top.trap_id < context->nodes.len);
        PathFindNode *curr_node = &array_at(&context->nodes, top.trap_id);
        curr_node->closed = true;

        PathTrapezoid *curr_trap = curr_node->point.trap;
        if (curr_trap == dst_trap) {
            (void) PathReversePathFindNode(curr_node);

            PathFindPoint src_point = {
                .pos = src_pos,
                .trap = src_trap,
            };

            PathFindPoint dst_point = {
                .pos = dst_pos,
                .trap = dst_trap,
            };

            #if 0
            while (it != NULL) {
                printf("Node %.4f, %.4f, %d\n", it->point.pos.x, it->point.pos.y, it->point.pos.plane);
                it = it->next;
            }
            #endif

            PathCreateWaypoints(context, src_point, dst_point, waypoints);
            return true;
        }

        // Not sure yet why we keep track of that...
        // float cur_dist = DistToTrap(dst_pos, curr_node->trap);
        // if (cur_dist < best_dist) {
        //     best_dist = cur_dist;

        //     dstSegment.trap = curr_node->trap;
        // }

        if (curr_trap->top_left) {
            // @Cleanup: Add array_get with assert.
            PathFindNode *node = &array_at(&context->nodes, curr_trap->top_left->trap_id);
            if (!node || !node->closed)
                PathVisitAbove(context, curr_node, dst_pos.v2, curr_trap->top_left, MAX_COST);
        }

        if (curr_trap->top_right) {
            PathFindNode *node = &array_at(&context->nodes, curr_trap->top_right->trap_id);
            if (!node || !node->closed)
                PathVisitAbove(context, curr_node, dst_pos.v2, curr_trap->top_right, MAX_COST);
        }

        if (curr_trap->bottom_left) {
            PathFindNode *node = &array_at(&context->nodes, curr_trap->bottom_left->trap_id);
            if (!node || !node->closed)
                PathVisitBellow(context, curr_node, dst_pos.v2, curr_trap->bottom_left, MAX_COST);
        }

        if (curr_trap->bottom_right) {
            PathFindNode *node = &array_at(&context->nodes, curr_trap->bottom_right->trap_id);
            if (!node || !node->closed)
                PathVisitBellow(context, curr_node, dst_pos.v2, curr_trap->bottom_right, MAX_COST);
        }

        if (curr_trap->portal_left != UINT16_MAX) {
            PathVisitPortalLeft(context, curr_node, dst_pos.v2, curr_trap->portal_left, MAX_COST);
        }

        if (curr_trap->portal_right != UINT16_MAX) {
            PathVisitPortalRight(context, curr_node, dst_pos.v2, curr_trap->portal_right, MAX_COST);
        }
    }

    return false;
}
