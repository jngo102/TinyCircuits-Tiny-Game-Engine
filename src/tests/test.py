import engine
import engine_draw
import engine_debug
import engine_input
import engine_physics
import engine_audio
from engine_physics import EmptyPolyCollisionShape2D, RectanglePolyCollisionShape2D, HexagonPolyCollisionShape2D
from engine_nodes import EmptyNode, Sprite2DNode, Rectangle2DNode, Polygon2DNode, Circle2DNode, CameraNode, VoxelSpaceNode, Physics2DNode, Text2DNode
from engine_math import Vector3, Vector2, Rectangle
from engine_resources import TextureResource, WaveSoundResource, FontResource
import math
import gc

# engine_debug.enable_all()
engine_debug.enable_setting(engine_debug.performance)
# font = FontResource("9pt-roboto-font.bmp")
# text = Text2DNode(text="H", font=font)


texture = TextureResource("32x32.bmp")


# circle = Circle2DNode()
# circle.outline = True
# circle.radius = 25
# rectangle = Rectangle2DNode()
# rectangle.width = 30
# rectangle.height = 15
# rectangle.outline = True
sprite = Sprite2DNode(texture=texture)
# polygon = Polygon2DNode()
# polygon.vertices.append(Vector2(-10, 10))
# polygon.vertices.append(Vector2(10, 10))
# polygon.vertices.append(Vector2(10, -10))
# polygon.vertices.append(Vector2(-10, -10))
# polygon.outline = True

# circle.color = 0b1111100000000000
# rectangle.color = 0b1111101001001001
# sprite.transparent_color = engine_draw.black

# circle.position = Vector2(0, -32)
# rectangle.position = Vector2(32, 0)
# sprite.position = Vector2(0, 32)
# polygon.position = Vector2(-32, 0)


class MyCam(CameraNode):
    def __init__(self):
        super().__init__(self)
    
    def tick(self):
        if engine_input.check_pressed(engine_input.A):
            self.zoom -= 0.01
        if engine_input.check_pressed(engine_input.B):
            self.zoom += 0.01
        
        if engine_input.check_pressed(engine_input.DPAD_UP):
            self.position.y -= 0.25
        if engine_input.check_pressed(engine_input.DPAD_DOWN):
            self.position.y += 0.25
        
        if engine_input.check_pressed(engine_input.DPAD_LEFT):
            self.position.x -= 0.25
        if engine_input.check_pressed(engine_input.DPAD_RIGHT):
            self.position.x += 0.25
        
        
        if engine_input.check_pressed(engine_input.BUMPER_LEFT):
            self.rotation.z += 0.01
        if engine_input.check_pressed(engine_input.BUMPER_RIGHT):
            self.rotation.z -= 0.01


camera = MyCam()
camera.zoom = 1


engine.start()