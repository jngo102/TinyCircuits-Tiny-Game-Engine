import engine_main

import engine
import engine_audio
from engine_resources import WaveSoundResource, TextureResource
from engine_nodes import Circle2DNode, Sprite2DNode, CameraNode

import time

cam = CameraNode()

time_before = time.ticks_ms()
wave = WaveSoundResource("15s_chirp.wav")
print("Load duration:", time.ticks_ms() - time_before, "ms")

time_before = time.ticks_ms()
texture = TextureResource("large.bmp")
print("Load duration:", time.ticks_ms() - time_before, "ms")

engine_audio.play(wave, 1, True)
spr = Sprite2DNode(texture=texture)

engine.start()