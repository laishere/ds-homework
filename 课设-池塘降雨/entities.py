from pygame import Surface
from math import *
from random import randrange
import numpy as np

import pygame

def clamp(a, minv, maxv):
    return max(minv, min(maxv, a))

class Entity:

    def __init__(self) -> None:
        self.t0 = None
        self.isInit = False
        self.isAlive = True

    def set_elevation(self, elevation):
        self.elevation = elevation
    
    def set_surf_name(self, name):
        self.surf_name = name

    def update(self, surface: Surface, t) -> tuple:
        if self.t0 is None:
            self.t0 = t
            self.lifetime = 0
        self.surface = surface
        self.t = t
        prev = self.lifetime 
        self.lifetime = t - self.t0 # lifetime是 (当前时间 - 实体第一次update的时间)
        self.dt = self.lifetime - prev # dt是 (此次update时间 - 前一次update时间)
        self._init() # 如果没有初始化，则初始化，此时已经有surface信息
        rect = self.update_rect() # 获取绘制区域
        return self.safe_rect(rect) # 确保区域合法和添加适当的额外边距
    
    def _init(self):
        if not self.isInit:
            self.init()
            self.isInit = True

    def init(self): ...

    def safe_rect(self, rect):
        w, h = self.surface.get_size()
        expand_edge = 5
        left, top, right, bottom = rect
        right = clamp(right + expand_edge, 0, w)
        bottom = clamp(bottom + expand_edge, 0, h)
        left = clamp(left - expand_edge, 0, right)
        top = clamp(top - expand_edge, 0, bottom)
        return (left, top, right - left, bottom - top)

    def update_rect(self) -> tuple: ...

    def draw(self) -> None: ...

    def circle(self, color, pos, r, width=0):
        pygame.draw.circle(self.surface, color, pos, r, width)

    def line(self, color, pos1, pos2, width=1):
        pygame.draw.line(self.surface, color, pos1, pos2, width)

class Ripple(Entity):

    def __init__(self, pos, color) -> None:
        super().__init__()
        self.pos = pos # 水波的中心位置
        self.color = color # 水波的颜色

    def init(self):
        self.alpha = 0.5 # 初始水波透明度

    def update_rect(self) -> tuple:
        self.x, self.y = self.pos
        self.r = self.lifetime * 40 # 水波半径随着时间递增
        self.rect = (self.x - self.r, self.y - self.r, self.x + self.r, self.y + self.r)
        return self.rect

    def draw(self):
        self.alpha -= 0.01
        alpha = max(0, self.alpha * 255) # 绘制透明度
        self.circle((*self.color, alpha), (self.x, self.y), self.r) # 绘制一个圆形水波
        if alpha == 0:
            # 如果透明为0，那么此水波应该被移除
            self.isAlive = False

class Rain(Entity):

    def init(self):
        w, h = self.surface.get_size()
        x = randrange(0, w)
        y = randrange(-h * 2, 0)
        self.pos1 = (x, y) # 初始位置
        self.z = randrange(0, h) # 雨滴深度信息
        r = (1 - self.z / h)
        a0, a1 = 0.1, 0.6
        self.alpha = (r * (a1 - a0) + a0) * 255 # 根据深度计算的透明度信息，越深越透明
        self.color = (randrange(0, 255), randrange(0, 255), randrange(0, 255)) # 随机雨滴颜色
        self.rain_spd = 500 # 默认雨滴速度
        self.win_spd = 0 # 默认风速

    def get_vel(self):
        return np.float32((self.win_spd, self.rain_spd)) # 获取雨滴的速度向量
    
    def update_rect(self) -> tuple:
        velocity = self.get_vel()
        x1, y1 = velocity * self.dt + self.pos1 # 位置变化
        x2, y2 = velocity * 0.1 + (x1, y1) # 计算雨滴下端点位置，速度越快，看到的雨滴越长
        self.pos1 = (x1, y1)
        self.pos2 = (x2, y2)
        return (min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2))

    def draw(self) -> None:
        self.line((*self.color, self.alpha), self.pos1, self.pos2) # 绘制一条雨滴线
