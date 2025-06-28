import arcade
import struct
import math

from dataclasses import dataclass
# from linux_process import *
from process import *
from pygw import *

class Trapezoid:
    @staticmethod
    def get_trapezoid_area(xtl, xtr, yt, xbl, xbr, yb):
        base_bot = abs(xbl - xbr)
        base_top = abs(xtl - xtr)
        height  = abs(yt - yb)
        return ((base_bot + base_top) * height) / 2

    def __init__(self, trap_id, xtl, xtr, yt, xbl, xbr, yb):
        self.trap_id = trap_id
        self.xtl = xtl
        self.xtr = xtr
        self.yt = yt
        self.xbl = xbl
        self.xbr = xbr
        self.yb = yb
        self.area = self.get_trapezoid_area(xtl, xtr, yt, xbl, xbr, yb)
        self.adjacents = []

    def reset_default_color(self):
        self.color = self.default_color

class Window(arcade.Window):
    WIDTH = 1280
    HEIGHT = 720

    COLOR1 = (0x46, 0x99, 0x90)
    COLOR2 = (0x91, 0x1e, 0xb4)

    def __init__(self, game, planes):
        super().__init__(Window.WIDTH, Window.HEIGHT, title="Walkable mesh")

        self.game = game
        self.planes = planes
        self.selected_trap_id = -1

        arcade.set_background_color(arcade.color.WHITE)

    def setup(self):
        self.detect_bounds()
        self.set_default_colors()
        self.create_lookup_dict()
        self.build_draw_list()

        self.camera = arcade.Camera(self.width, self.height)
        self.hud_camera = arcade.Camera(self.width, self.height)

    def detect_bounds(self):
        self.min_x = float('inf')
        self.min_y = float('inf')
        self.max_x = float('-inf')
        self.max_y = float('-inf')

        for zplane, traps, x_nodes, y_nodes in self.planes:
            for trap in traps:
                self.min_x = min(self.min_x, trap.xtl, trap.xbl)
                self.max_x = max(self.max_x, trap.xtr, trap.xbr)
                self.min_y = min(self.min_y, trap.yb)
                self.max_y = max(self.max_y, trap.yt)

        self.offset_x = -self.min_x
        self.offset_y = -self.min_y
        self.ratio_w  = Window.WIDTH / (self.max_x - self.min_x)
        self.ratio_h  = Window.HEIGHT / (self.max_y - self.min_y)

    def set_default_colors(self):
        for idx, (plane, traps, x_nodes, y_nodes) in enumerate(self.planes):
            if idx == 0:
                default_color = self.COLOR1
            else:
                default_color = self.COLOR2
            for trap in traps:
                trap.color = default_color
                trap.default_color = default_color
                # if trap.trap_id == 8:
                #     trap.color = arcade.color.YELLOW

    def create_lookup_dict(self):
        self.lookup = {}
        for idx, (plane, traps, x_nodes, y_nodes) in enumerate(self.planes):
            for trap in traps:
                self.lookup[trap.trap_id] = trap

    @staticmethod
    def get_triangle_area_from_points(ax, ay, bx, by, cx, cy):
        square_area = ax * (by - cy) + bx * (cy - ay) + cx * (ay - by)
        return abs(square_area) / 2

    @staticmethod
    def get_triangle_are_from_base_height(xl, xy, y1, y2):
        base = abs(xl - xy)
        height = abs(y1 - y2)
        return (base * height) / 2

    @staticmethod
    def is_in_trapezoid(trap, x, y):
        at = Window.get_triangle_are_from_base_height(trap.xtl, trap.xtr, trap.yt, y)
        ab = Window.get_triangle_are_from_base_height(trap.xbl, trap.xbr, trap.yb, y)
        al = Window.get_triangle_area_from_points(x, y, trap.xtl, trap.yt, trap.xbl, trap.yb)
        ar = Window.get_triangle_area_from_points(x, y, trap.xtr, trap.yt, trap.xbr, trap.yb)
        return (at + ab + al + ar) <= trap.area

    def find_trapezoid_by_coord(self, x, y):
        for plane, traps, x_nodes, y_nodes in reversed(self.planes):
            for trap in traps:
                if y < trap.yb or trap.yt < y:
                    continue
                if self.is_in_trapezoid(trap, x, y):
                    print(f'Found trap in plane {plane}')
                    return trap

    def window_coord_to_world_coord(self, x, y):
        x = (x / self.ratio_w) - self.offset_x
        y = (y / self.ratio_h) - self.offset_y
        return x, y

    def world_coord_to_window_coord(self, x, y):
        x = (x + self.offset_x) * self.ratio_w
        y = (y + self.offset_y) * self.ratio_h
        return (x, y)

    def build_draw_list(self):
        elems = arcade.ShapeElementList()
        for idx, (plane, traps, x_nodes, y_nodes) in enumerate(self.planes):
            for trap in traps:
                xtl = (trap.xtl + self.offset_x) * self.ratio_w
                xtr = (trap.xtr + self.offset_x) * self.ratio_w
                xbl = (trap.xbl + self.offset_x) * self.ratio_w
                xbr = (trap.xbr + self.offset_x) * self.ratio_w
                yt  = (trap.yt + self.offset_y) * self.ratio_h
                yb  = (trap.yb + self.offset_y) * self.ratio_h

                colors = (trap.color, trap.color, trap.color, trap.color, trap.color, trap.color)
                points = ((xbl, yb), (xtr, yt), (xtl, yt), (xbl, yb), (xtr, yt), (xbr, yb))
                elems.append(arcade.create_triangles_filled_with_colors(points, colors))
                # elems.append(arcade.create_ellipse_filled(xbl, yb, 3, 3, arcade.color.BLUE))
                # elems.append(arcade.create_ellipse_filled(xtr, yt, 3, 3, arcade.color.BLUE))
                # elems.append(arcade.create_ellipse_filled(xtl, yt, 3, 3, arcade.color.BLUE))
                # elems.append(arcade.create_ellipse_filled(xbr, yb, 3, 3, arcade.color.BLUE))

        for idx, (plane, traps, x_nodes, y_nodes) in enumerate(self.planes):
            for jdx, (x1, y1, x2, y2) in enumerate(x_nodes):
                x1 = (x1 + self.offset_x) * self.ratio_w
                y1 = (y1 + self.offset_y) * self.ratio_h
                x2 = (x2 + self.offset_x) * self.ratio_w
                y2 = (y2 + self.offset_y) * self.ratio_h
                elems.append(arcade.create_line(x1, y1, x2, y2, arcade.color.GREEN, 2))

            for x, y in y_nodes:
                x = (x + self.offset_x) * self.ratio_w
                y = (y + self.offset_y) * self.ratio_h
                elems.append(arcade.create_ellipse_filled(x, y, 1, 1, arcade.color.RED))

        self.elems = elems

    def on_draw(self):
        px, py, plane = self.game.get_agent_pos(self.game.get_agent_id_by_player_id())
        x, y = self.world_coord_to_window_coord(px, py)

        arcade.start_render()
        self.clear()
        self.camera.use()

        self.elems.draw()

        def draw_trap(trap, color):
            xtl = (trap.xtl + self.offset_x) * self.ratio_w
            xtr = (trap.xtr + self.offset_x) * self.ratio_w
            xbl = (trap.xbl + self.offset_x) * self.ratio_w
            xbr = (trap.xbr + self.offset_x) * self.ratio_w
            yt  = (trap.yt + self.offset_y) * self.ratio_h
            yb  = (trap.yb + self.offset_y) * self.ratio_h
            arcade.draw_triangle_filled(xbl, yb, xtr, yt, xtl, yt, color)
            arcade.draw_triangle_filled(xbl, yb, xtr, yt, xbr, yb, color)

        if self.selected_trap_id in self.lookup:
            trap = self.lookup[self.selected_trap_id]
            draw_trap(trap, arcade.color.BLACK)

            """
            adjacent_colors = (arcade.color.RED, arcade.color.GREEN, arcade.color.BLUE, arcade.color.YELLOW)
            for adj, col in zip(trap.adjacents, adjacent_colors):
                if adj == -1:
                    continue
                trap = self.lookup[adj]
                draw_trap(trap, col)
            """

        ox, oy = self.world_coord_to_window_coord(0, 0)

        arcade.draw_circle_filled(ox, oy, 4, arcade.color.ORANGE)
        arcade.draw_circle_filled(x, y, 8, arcade.color.GREEN)

        self.hud_camera.use()
        arcade.draw_text(f'pos: ({px:.06}, {py:.06})', 10, 10, arcade.color.BLACK, 15, width=self.width, align="left")

    def on_mouse_press(self, x, y, button, modifiers):
        scale = self.camera.scale
        dx, dy = self.camera.position
        x = (self.width / 2) * (1 - scale) + (x * scale) + dx
        y = (self.height / 2) * (1 - scale) + (y * scale) + dy

        x, y = self.window_coord_to_world_coord(x, y)
        trap = self.find_trapezoid_by_coord(x, y)
        if trap != None:
            self.selected_trap_id = trap.trap_id
        else:
            self.selected_trap_id = -1

    def on_mouse_drag(self, x, y, dx, dy, buttons, modifiers):
        dx *= self.camera.scale
        dy *= self.camera.scale
        pos_x, pos_y = self.camera.position
        self.camera.move((pos_x - dx, pos_y - dy))

    def on_mouse_scroll(self, x, y, scroll_x, scroll_y):
        SCROLL_SPEED = 0.2
        self.camera.scale = max(0.1, min(10, self.camera.scale - (scroll_y * SCROLL_SPEED)))

def build_visibility_graph(traps):
    def area(a, b, c):
        ax, ay = a
        bx, by = b
        cx, cy = c
        return (bx - ax) * (cy - ay) * (cx - ax) * (by - ay)

    def is_crossed_by(trap, p1, p2):
        xtl, xtr, yt, xbl, xbr, yb = trap
        left = (xtl, yt), (xbl, yb)
        right = (xtr, yt), (xbr, yb)

    def is_in_line_of_sight(p1, p2):
        for trap in traps:
            if is_crossed_by(trap, p1, p2):
                return False
        return True

    points = []
    for xtl, xtr, yt, xbl, xbr, yb in traps:
        points.append((xbl, yb))
        points.append((xtr, yt))
        points.append((xtl, yt))
        points.append((xbr, yb))

    graph = {}
    for idx, p1 in enumerate(points):
        links = []
        for jdx, p2 in enumerate(points):
            if idx == jdx:
                continue
            if is_in_line_of_sight(p1, p2):
                links.append(p2)
        graph[p1] = links
    return graph

if __name__ == '__main__':
    proc, = GetProcesses('Gw.exe')
    game = pygw(proc)
    pathing_maps = game.get_pathing_maps()

    SIZE = 0x54
    bytes = pathing_maps.len * SIZE
    data, = proc.read(pathing_maps.buf, f'{bytes}s')

    planes = []
    for idx in range(pathing_maps.len):
        zplane, = struct.unpack_from('<I', data, (idx * SIZE) + 0)
        traps = []

        trap_len, trap_ptr = struct.unpack_from('<II', data, (idx * SIZE) + 0x14)
        traps_bytes, = proc.read(trap_ptr, f'{trap_len * 48}s')
        traps = []
        for jdx in range(trap_len):
            trap_id, *adjacents = struct.unpack_from('<IIIII', traps_bytes, jdx * 48)
            xtl, xtr, yt, xbl, xbr, yb = struct.unpack_from('<ffffff', traps_bytes, jdx * 48 + 0x18)

            trap = Trapezoid(trap_id, xtl, xtr, yt, xbl, xbr, yb)
            for adjacent in adjacents:
                if adjacent != 0:
                    trap.adjacents.append(proc.read(adjacent)[0])
                else:
                    trap.adjacents.append(-1)
            traps.append(trap)
        """
        trap_len, trap_ptr = struct.unpack_from('<II', data, (idx * SIZE) + 0x34)
        trap_ptrs = proc.read(trap_ptr, f'{trap_len}I')
        for trap_ptr in trap_ptrs:
            trap_id, *adjacents = proc.read(trap_ptr, '<IIIII')
            xtl, xtr, yt, xbl, xbr, yb = proc.read(trap_ptr + 0x18, '<ffffff')
            trap = Trapezoid(trap_id, xtl, xtr, yt, xbl, xbr, yb)
            traps.append(trap)
        """

        x_nodes = []
        """
        x_node_count, x_node_ptr = struct.unpack_from('<II', data, (idx * SIZE) + 0x24)
        x_node_bytes, = proc.read(x_node_ptr, f'{x_node_count * 32}s')
        for jdx in range(x_node_count):
            x1, y1, x2, y2 = struct.unpack_from('<ffff', x_node_bytes, (jdx * 32) + 8)
            left, right = struct.unpack_from('<II', x_node_bytes, (jdx * 32) + 0x18)
            x_nodes.append((x1, y1, x1 + x2, y1 + y2))
        """

        y_nodes = []
        """
        y_node_count, y_node_ptr = struct.unpack_from('<II', data, (idx * SIZE) + 0x2C)
        y_node_bytes, = proc.read(y_node_ptr, f'{y_node_count * 24}s')
        for jdx in range(y_node_count):
            x, y, left, right = struct.unpack_from('<ffII', y_node_bytes, (jdx * 24) + 8)
            y_nodes.append((x, y))
        """

        portal_count, portal_ptr = struct.unpack_from('<II', data, (idx * SIZE) + 0x3C)
        portals_size = portal_count * 0x14
        portals_bytes, = proc.read(portal_ptr, f'{portals_size}s')
        for jdx in range(portal_count):
            left_layer_id, right_layer_id, other = struct.unpack_from('<HHI', portals_bytes, jdx * 0x14)
            # print(f'left_layer_id = {left_layer_id}, right_layer_id = {right_layer_id}, other = {other:X}')
            # tc, tp = struct.unpack_from('<II', portals_bytes, (jdx * 0x14) + 0xC)
            # ptrs = proc.read(tp, f'{tc}I')
            # for ptr in ptrs:
            #     trap_id, = proc.read(ptr)
            #     xtl, xtr, yt, xbl, xbr, yb = proc.read(ptr + 0x18, 'ffffff')
            #     trap = Trapezoid(trap_id, xtl, xtr, yt, xbl, xbr, yb)
            #     traps.append(trap)

        planes.append((zplane, traps, x_nodes, y_nodes))

    window = Window(game, planes)
    window.setup()

    # Keep the window up until someone closes it.
    arcade.run()
